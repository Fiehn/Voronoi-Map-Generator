#include <glad/glad.h>
#include "GlobeRenderer.hpp"
#include "GLContext.hpp"
#include "MapProjection.hpp"
#include <SFML/Window/Context.hpp>
#include "Logger.h"
#include <cmath>
#include <cstddef>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

namespace {
    // uProjection 0 is the globe, the others are the Projection values. The flat branch repeats
    // mapproj::project, keep the two in step
    const char* VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aLatLon;
uniform int uProjection;
uniform mat4 uViewProj;   // Globe: sphere to clip space. Flat: map to clip space
uniform float uLift;      // Globe: pushes overlays off the surface so they don't z-fight with it
uniform float uFirstCopy; // Flat: instance i draws the map shifted by uFirstCopy + i world widths
uniform float uMaxY;      // Flat: the map ends at |y| = uMaxY
uniform float uMaxX;      // Flat: and at |x| = uMaxX when it doesn't wrap
out vec4 vColor;

const float PI = 3.14159265358979;
const float POLE_CLAMP = 1.57062178; // 89.99 degrees

void main()
{
    vColor = aColor;
    if (uProjection == 0) {
        gl_Position = uViewProj * vec4(aPosition * uLift, 1.0);
        gl_ClipDistance[0] = 1.0;
        gl_ClipDistance[1] = 1.0;
        gl_ClipDistance[2] = 1.0;
        gl_ClipDistance[3] = 1.0;
        return;
    }

    vec2 map = vec2(aLatLon.y, aLatLon.x);
    if (uProjection == 1) {
        float lat = clamp(aLatLon.x, -POLE_CLAMP, POLE_CLAMP);
        map.y = log(tan(0.25 * PI + 0.5 * lat));
    }
    map.x += (uFirstCopy + float(gl_InstanceID)) * 2.0 * PI;
    gl_Position = uViewProj * vec4(map, 0.0, 1.0);
    // The cut and the edges are straight lines on the map, so clipping against them is exact
    gl_ClipDistance[0] = uMaxY - map.y;
    gl_ClipDistance[1] = uMaxY + map.y;
    gl_ClipDistance[2] = uMaxX - map.x;
    gl_ClipDistance[3] = uMaxX + map.x;
}
)";

    const char* FRAGMENT_SHADER = R"(
#version 330 core
in vec4 vColor;
out vec4 fragColor;
void main()
{
    fragColor = vColor;
}
)";

    constexpr float HIGHLIGHT_LIFT = 1.001f;

    // (lat, lon) in degrees, in double so a ring vertex shared by several cells gets the same
    // answer to the last bit in each of them
    glm::dvec2 latLonOf(const glm::vec3& p)
    {
        const double x = p.x, y = p.y, z = p.z;
        return glm::dvec2(glm::degrees(std::atan2(y, std::sqrt(x * x + z * z))), glm::degrees(std::atan2(x, z)));
    }

    glm::vec2 toRadians(double lat, double lon)
    {
        return glm::vec2(static_cast<float>(glm::radians(lat)), static_cast<float>(glm::radians(lon)));
    }
}

PlanetMesh buildPlanetMesh(const vor::SphereVoronoi& sphere)
{
    PlanetMesh mesh;
    const int n = static_cast<int>(sphere.cells.size());
    if (n == 0) return mesh;

    // The pole lies in the cell with the nearest centre, which is what findCell returns
    const int north = sphere.findCell(glm::vec3(0.f, 1.f, 0.f));
    int south = sphere.findCell(glm::vec3(0.f, -1.f, 0.f), north);
    if (south == north) south = -1; // Only with a handful of cells

    // Centre then ring for every cell, fanned into triangles that stay counter clockwise
    std::vector<unsigned int> poleFans, otherFans;
    mesh.cellFirstVertex.resize(n + 1);
    double lonMin = 0.0, lonMax = 0.0;
    for (int i = 0; i < n; i++) {
        const bool pole = i == north || i == south;
        const std::vector<int>& ring = sphere.cells[i].vertex;
        const unsigned int centre = static_cast<unsigned int>(mesh.positions.size());
        mesh.cellFirstVertex[i] = static_cast<int>(centre);

        const glm::dvec2 centreLL = latLonOf(sphere.points[i]);
        mesh.positions.push_back(sphere.points[i]);
        mesh.latLon.push_back(toRadians(centreLL.x, centreLL.y));
        for (int t : ring) {
            const glm::dvec2 ll = latLonOf(sphere.voronoi_points[t]);
            const double lon = mapproj::unwrapLon(ll.y, centreLL.y);
            mesh.positions.push_back(sphere.voronoi_points[t]);
            mesh.latLon.push_back(toRadians(ll.x, lon));
            if (!pole) {
                lonMin = std::min(lonMin, lon);
                lonMax = std::max(lonMax, lon);
            }
        }

        std::vector<unsigned int>& fans = pole ? poleFans : otherFans;
        const unsigned int size = static_cast<unsigned int>(ring.size());
        for (unsigned int k = 0; k < size; k++) {
            fans.push_back(centre);
            fans.push_back(centre + 1 + k);
            fans.push_back(centre + 1 + (k + 1) % size);
        }
    }
    mesh.cellFirstVertex[n] = static_cast<int>(mesh.positions.size());

    // Pole bands. Walk the ring with each step's longitude unwrapped from the step before; a
    // ring edge never spans 180 degrees of longitude unless it runs through the pole, so the
    // walk ends exactly one turn from where it began. Each ring vertex gets a partner straight
    // above it on the pole, and every edge becomes the quad between the two pairs
    std::vector<unsigned int> bands;
    const auto addBand = [&](int cell, double poleLat) {
        const std::vector<int>& ring = sphere.cells[cell].vertex;
        const int size = static_cast<int>(ring.size());
        const unsigned int first = static_cast<unsigned int>(mesh.positions.size());
        const glm::vec3 polePoint(0.f, poleLat > 0.0 ? 1.f : -1.f, 0.f);

        double lon = 0.0;
        for (int k = 0; k <= size; k++) {
            const glm::vec3& p = sphere.voronoi_points[ring[k % size]];
            const glm::dvec2 ll = latLonOf(p);
            lon = k == 0 ? ll.y : mapproj::unwrapLon(ll.y, lon);
            mesh.positions.push_back(p);
            mesh.latLon.push_back(toRadians(ll.x, lon));
            mesh.positions.push_back(polePoint);
            mesh.latLon.push_back(toRadians(poleLat, lon));
            lonMin = std::min(lonMin, lon);
            lonMax = std::max(lonMax, lon);
        }
        const double turn = lon - latLonOf(sphere.voronoi_points[ring[0]]).y;
        if (std::abs(std::abs(turn) - 360.0) > 1e-6) {
            LOG_WARNING(Graphics, "Pole cell ring does not go round the pole once, cell and degrees:", cell, static_cast<float>(turn));
        }

        for (int k = 0; k < size; k++) {
            const unsigned int ringK = first + 2 * k, poleK = ringK + 1, ringNext = ringK + 2, poleNext = ringK + 3;
            bands.insert(bands.end(), { ringK, ringNext, poleNext, ringK, poleNext, poleK });
        }
        mesh.poleBands.push_back({ cell, static_cast<int>(first), 2 * (size + 1) });
    };
    addBand(north, 90.0);
    if (south >= 0) addBand(south, -90.0);

    mesh.indices = poleFans;
    mesh.indices.insert(mesh.indices.end(), otherFans.begin(), otherFans.end());
    mesh.indices.insert(mesh.indices.end(), bands.begin(), bands.end());
    mesh.poleFanIndexCount = static_cast<int>(poleFans.size());
    mesh.globeIndexCount = static_cast<int>(poleFans.size() + otherFans.size());
    mesh.lonMin = static_cast<float>(glm::radians(lonMin));
    mesh.lonMax = static_cast<float>(glm::radians(lonMax));
    return mesh;
}

std::vector<OutlineVertex> buildCellOutline(const vor::SphereVoronoi& sphere, int cell)
{
    std::vector<OutlineVertex> outline;
    if (cell < 0 || cell >= static_cast<int>(sphere.cells.size())) return outline;
    const std::vector<int>& ring = sphere.cells[cell].vertex;
    const int size = static_cast<int>(ring.size());
    if (size == 0) return outline;

    const double firstLon = latLonOf(sphere.voronoi_points[ring[0]]).y;
    double lon = latLonOf(sphere.points[cell]).y;
    std::vector<glm::dvec2> latLon;
    for (int k = 0; k <= size; k++) {
        const glm::dvec2 ll = latLonOf(sphere.voronoi_points[ring[k % size]]);
        lon = mapproj::unwrapLon(ll.y, lon);
        latLon.push_back(glm::dvec2(ll.x, lon));
    }
    // A pole cell's centre longitude means nothing, start it where its band starts instead so
    // it stays inside the longitude range the flat view draws copies for
    const bool pole = std::abs(latLon.back().y - latLon.front().y) > 180.0;
    const double shift = pole ? firstLon - latLon.front().y : 0.0;
    for (int k = 0; k <= size; k++) {
        outline.push_back({ sphere.voronoi_points[ring[k % size]], toRadians(latLon[k].x, latLon[k].y + shift) });
    }
    return outline;
}

GlobeRenderer::~GlobeRenderer()
{
    release();
    if (program && contextActive()) glDeleteProgram(program);
}

bool GlobeRenderer::contextActive() const
{
    return contextId != 0 && sf::Context::getActiveContextId() == contextId;
}

void GlobeRenderer::release()
{
    if (contextActive()) {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (highlightVao) glDeleteVertexArrays(1, &highlightVao);
        GLuint buffers[] = { positionBuffer, latLonBuffer, colorBuffer, indexBuffer, highlightBuffer };
        for (GLuint b : buffers) {
            if (b) glDeleteBuffers(1, &b);
        }
    }
    vao = highlightVao = positionBuffer = latLonBuffer = colorBuffer = indexBuffer = highlightBuffer = 0;
    indexCount = poleFanIndexCount = globeIndexCount = highlightCount = 0;
    cellFirstVertex.clear();
    poleBands.clear();
}

void GlobeRenderer::build(const vor::SphereVoronoi& sphere)
{
    release();

    const unsigned long long active = sf::Context::getActiveContextId();
    if (active == 0) return;
    if (active != contextId) program = 0; // Made in another context, which can't use it here
    contextId = active;

    if (!program) {
        program = compileProgram(VERTEX_SHADER, FRAGMENT_SHADER);
        if (!program) return;
        viewProjLocation = glGetUniformLocation(program, "uViewProj");
        liftLocation = glGetUniformLocation(program, "uLift");
        projectionLocation = glGetUniformLocation(program, "uProjection");
        firstCopyLocation = glGetUniformLocation(program, "uFirstCopy");
        maxYLocation = glGetUniformLocation(program, "uMaxY");
        maxXLocation = glGetUniformLocation(program, "uMaxX");
    }

    PlanetMesh mesh = buildPlanetMesh(sphere);
    if (mesh.indices.empty()) return;
    cellFirstVertex = std::move(mesh.cellFirstVertex);
    poleBands = std::move(mesh.poleBands);
    poleFanIndexCount = mesh.poleFanIndexCount;
    globeIndexCount = mesh.globeIndexCount;
    lonMin = mesh.lonMin;
    lonMax = mesh.lonMax;
    vertexColors.assign(mesh.positions.size(), sf::Color(128, 128, 128));

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(glm::vec3), mesh.positions.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    // sf::Color is four bytes, r g b a, which is exactly a normalised ubyte4 attribute
    glGenBuffers(1, &colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexColors.size() * sizeof(sf::Color), vertexColors.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(sf::Color), nullptr);

    glGenBuffers(1, &latLonBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, latLonBuffer);
    glBufferData(GL_ARRAY_BUFFER, mesh.latLon.size() * sizeof(glm::vec2), mesh.latLon.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(GLuint), mesh.indices.data(), GL_STATIC_DRAW);
    indexCount = static_cast<int>(mesh.indices.size());

    // The highlight outline, positions and lat/lon interleaved; its colour is a constant attribute
    glGenVertexArrays(1, &highlightVao);
    glBindVertexArray(highlightVao);
    glGenBuffers(1, &highlightBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, highlightBuffer);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OutlineVertex), reinterpret_cast<const void*>(offsetof(OutlineVertex, position)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(OutlineVertex), reinterpret_cast<const void*>(offsetof(OutlineVertex, latLon)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    LOG_INFO(Graphics, "Planet mesh built, vertices, triangles and pole bands:", static_cast<int>(mesh.positions.size()), indexCount / 3, static_cast<int>(poleBands.size()));
}

void GlobeRenderer::setCellColors(const std::vector<sf::Color>& colors)
{
    if (!ready() || colors.size() + 1 != cellFirstVertex.size()) return;

    for (std::size_t i = 0; i < colors.size(); i++) {
        std::fill(vertexColors.begin() + cellFirstVertex[i], vertexColors.begin() + cellFirstVertex[i + 1], colors[i]);
    }
    for (const PlanetMesh::Band& band : poleBands) {
        std::fill_n(vertexColors.begin() + band.firstVertex, band.vertexCount, colors[band.cell]);
    }
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexColors.size() * sizeof(sf::Color), vertexColors.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlobeRenderer::setHighlight(const vor::SphereVoronoi& sphere, int cell)
{
    if (!ready()) return;
    const std::vector<OutlineVertex> outline = buildCellOutline(sphere, cell);
    highlightCount = static_cast<int>(outline.size());
    if (outline.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, highlightBuffer);
    glBufferData(GL_ARRAY_BUFFER, outline.size() * sizeof(OutlineVertex), outline.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlobeRenderer::draw(const PlanetCamera& camera, const glm::vec2& viewport)
{
    if (!ready() || viewport.x <= 0.f || viewport.y <= 0.f) return;

    glViewport(0, 0, static_cast<GLsizei>(viewport.x), static_cast<GLsizei>(viewport.y));
    // Same blending as SFML's BlendAlpha, the height colours fade towards the background with their alpha
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program);

    if (camera.mode == ViewMode::Flat) drawFlat(camera, viewport);
    else drawGlobe(camera, viewport);

    glBindVertexArray(0);
    glUseProgram(0);
}

void GlobeRenderer::drawGlobe(const PlanetCamera& camera, const glm::vec2& viewport)
{
    const glm::mat4 viewProj = camera.globeViewProj(viewport);

    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glUniform1i(projectionLocation, 0);
    glUniformMatrix4fv(viewProjLocation, 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform1f(liftLocation, 1.f);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, globeIndexCount, GL_UNSIGNED_INT, nullptr);

    if (highlightCount > 0) {
        glUniform1f(liftLocation, HIGHLIGHT_LIFT);
        glBindVertexArray(highlightVao);
        glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
        glLineWidth(2.f);
        glDrawArrays(GL_LINE_STRIP, 0, highlightCount);
        glLineWidth(1.f);
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void GlobeRenderer::drawFlat(const PlanetCamera& camera, const glm::vec2& viewport)
{
    // Copy k of the map is shifted by k world widths. Draw every copy whose longitude range
    // [lonMin, lonMax] + k turns reaches into the half width either side of the centre
    const float turn = glm::two_pi<float>();
    const float halfWidth = 0.5f * viewport.x / camera.flatScale();
    const float centreX = camera.flatCentre().x;
    int firstCopy = static_cast<int>(std::ceil((centreX - halfWidth - lonMax) / turn));
    int lastCopy = static_cast<int>(std::floor((centreX + halfWidth - lonMin) / turn));
    // A static map is the copies either side too, clipped at 180W and 180E, so the cells
    // across the 180th meridian fill its edges. No vertex is more than a turn from its copy
    const bool wraps = camera.flatWraps();
    if (!wraps) {
        firstCopy = std::max(firstCopy, -1);
        lastCopy = std::min(lastCopy, 1);
    }
    if (lastCopy < firstCopy) return;

    const glm::mat4 viewProj = camera.flatViewProj(viewport);
    const float maxY = mapproj::project(camera.projection, mapproj::maxLat(camera.projection), 0.f).y;
    // Wrapping, the x clip planes sit well outside every copy drawn
    const float maxX = wraps ? turn * (2.f + std::max(std::abs(firstCopy), std::abs(lastCopy))) : glm::pi<float>();

    // Flat cells don't overlap and keep their winding, so no depth test and no culling
    const GLenum clipPlanes[] = { GL_CLIP_DISTANCE0, GL_CLIP_DISTANCE1, GL_CLIP_DISTANCE2, GL_CLIP_DISTANCE3 };
    for (GLenum plane : clipPlanes) glEnable(plane);
    glUniform1i(projectionLocation, static_cast<int>(camera.projection));
    glUniformMatrix4fv(viewProjLocation, 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform1f(firstCopyLocation, static_cast<float>(firstCopy));
    glUniform1f(maxYLocation, maxY);
    glUniform1f(maxXLocation, maxX);
    glBindVertexArray(vao);
    const std::size_t offset = static_cast<std::size_t>(poleFanIndexCount) * sizeof(GLuint);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount - poleFanIndexCount, GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(offset), lastCopy - firstCopy + 1);

    // The outline lies within the mesh's longitude range, so the same copies cover it
    if (highlightCount > 0) {
        glBindVertexArray(highlightVao);
        glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
        glLineWidth(2.f);
        glDrawArraysInstanced(GL_LINE_STRIP, 0, highlightCount, lastCopy - firstCopy + 1);
        glLineWidth(1.f);
    }

    for (GLenum plane : clipPlanes) glDisable(plane);
}
