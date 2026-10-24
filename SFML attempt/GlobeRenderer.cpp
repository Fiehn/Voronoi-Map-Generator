#include <glad/glad.h>
#include "GlobeRenderer.hpp"
#include "GLContext.hpp"
#include "MapProjection.hpp"
#include "SphereMath.hpp"
#include "CellObjects.hpp"
#include <SFML/Window/Context.hpp>
#include "Logger.h"
#include <cmath>
#include <cstddef>
#include <string>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

namespace {
    // Shared by every vertex shader. uProjection 0 is the globe, the others are the Projection
    // values. mapPoint repeats mapproj::project, keep the two in step
    const char* PROJECTION_GLSL = R"(
uniform int uProjection;
uniform mat4 uViewProj;   // Globe: sphere to clip space. Flat: map to clip space
uniform float uLift;      // Globe: pushes the debug overlays off the surface so they don't z-fight with it
uniform vec3 uEye;        // Globe: eye position
uniform int uHorizonClip; // Globe: clip what lies beyond the horizon, for overlays drawn without the depth test
uniform float uFirstCopy; // Flat: instance i draws the map shifted by uFirstCopy + i world widths
uniform float uMaxY;      // Flat: the map ends at |y| = uMaxY
uniform float uMaxX;      // Flat: and at |x| = uMaxX
uniform vec2 uViewport;   // Pixels, for the overlays sized on screen

const float PI = 3.14159265358979;
const float POLE_CLAMP = 1.57062178; // 89.99 degrees

// Pixels from the screen centre of a clip space point
vec2 toPixels(vec4 clip) { return clip.xy / clip.w * 0.5 * uViewport; }
// Moves a clip space position by a pixel offset
vec4 offsetPixels(vec4 clip, vec2 pixels) { return vec4(clip.xy + pixels * 2.0 / uViewport * clip.w, clip.zw); }

// Flat map point of this instance's copy
vec2 mapPoint(vec2 latLon)
{
    vec2 map = vec2(latLon.y, latLon.x);
    if (uProjection == 1) {
        float lat = clamp(latLon.x, -POLE_CLAMP, POLE_CLAMP);
        map.y = log(tan(0.25 * PI + 0.5 * lat));
    }
    map.x += (uFirstCopy + float(gl_InstanceID)) * 2.0 * PI;
    return map;
}

vec4 projectPoint(vec3 position, vec2 latLon)
{
    if (uProjection == 0) return uViewProj * vec4(position * uLift, 1.0);
    return uViewProj * vec4(mapPoint(latLon), 0.0, 1.0);
}

void setClipDistances(vec3 position, vec2 latLon)
{
    if (uProjection == 0) {
        // A unit sphere point is in front of the horizon when dot(p, eye) > 1
        gl_ClipDistance[0] = uHorizonClip != 0 ? dot(position, uEye) - 1.0 : 1.0;
        gl_ClipDistance[1] = 1.0;
        gl_ClipDistance[2] = 1.0;
        gl_ClipDistance[3] = 1.0;
        return;
    }
    // The cut and the edges are straight lines on the map, so clipping against them is exact
    vec2 map = mapPoint(latLon);
    gl_ClipDistance[0] = uMaxY - map.y;
    gl_ClipDistance[1] = uMaxY + map.y;
    gl_ClipDistance[2] = uMaxX - map.x;
    gl_ClipDistance[3] = uMaxX + map.x;
}
)";

    // Cells and the debug overlays
    const char* MESH_VERTEX_MAIN = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aLatLon;
layout(location = 3) in float aPointSize; // Pixels, for the debug points
out vec4 vColor;

void main()
{
    vColor = aColor;
    gl_PointSize = aPointSize;
    gl_Position = projectPoint(aPosition, aLatLon);
    setClipDistances(aPosition, aLatLon);
}
)";

    // Ribbon lines, see Ribbon.hpp. The previous, current and next point come from the same
    // buffer at three offsets. The miter is worked out in pixels, so the width stays constant
    const char* RIBBON_VERTEX_MAIN = R"(
layout(location = 0) in vec3 aPrevPosition;
layout(location = 1) in vec2 aPrevLatLon;
layout(location = 2) in vec3 aPosition;
layout(location = 3) in vec2 aLatLon;
layout(location = 4) in vec3 aNextPosition;
layout(location = 5) in vec2 aNextLatLon;
layout(location = 6) in vec4 aColor;
layout(location = 7) in float aHalfWidth;
layout(location = 8) in float aSide;
uniform int uDiscs; // Round points of diameter 2 * aHalfWidth instead of lines
out vec4 vColor;

const float MITER_LIMIT = 2.0; // Max miter length in half widths

void main()
{
    vColor = aColor;
    vec4 clip = projectPoint(aPosition, aLatLon);
    setClipDistances(aPosition, aLatLon);
    gl_PointSize = 2.0 * aHalfWidth;
    gl_Position = clip;
    if (uDiscs != 0) return;

    vec2 cur = toPixels(clip);
    vec2 dIn = cur - toPixels(projectPoint(aPrevPosition, aPrevLatLon));
    vec2 dOut = toPixels(projectPoint(aNextPosition, aNextLatLon)) - cur;
    // An end of an open line has only one direction, a butt end
    if (dot(dIn, dIn) < 1e-12) dIn = dOut;
    if (dot(dOut, dOut) < 1e-12) dOut = dIn;
    if (dot(dOut, dOut) < 1e-12) return;
    dIn = normalize(dIn);
    dOut = normalize(dOut);
    vec2 nIn = vec2(-dIn.y, dIn.x);
    vec2 nOut = vec2(-dOut.y, dOut.x);

    vec2 offset = nOut * aHalfWidth;
    vec2 miter = nIn + nOut;
    float len = length(miter);
    // A line that turns back on itself (a river retracing a branch) has no miter, it keeps the outgoing normal
    if (len > 1e-4) {
        miter /= len;
        offset = miter * (aHalfWidth / max(dot(miter, nOut), 1.0 / MITER_LIMIT));
    }
    gl_Position = offsetPixels(clip, offset * aSide);
}
)";

    // Arrows, see ArrowField.hpp. Each corner knows the arrow's anchor and a point a little way
    // ahead of it, so the arrow points along the projected direction on any view. The shape is
    // the flat map's: the tip one length ahead, the base corners 0.7 lengths back at 22.5 degrees
    const char* ARROW_VERTEX_MAIN = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aLatLon;
layout(location = 2) in vec3 aAheadPosition;
layout(location = 3) in vec2 aAheadLatLon;
layout(location = 4) in vec4 aColor;
layout(location = 5) in float aSize;   // Length as a fraction of uSlot
layout(location = 6) in float aCorner; // 0 tip, 1 and 2 the base corners
uniform float uSlot; // Pixels between neighbouring arrows
out vec4 vColor;

const float SPREAD = PI / 8.0;

void main()
{
    vColor = aColor;
    vec4 clip = projectPoint(aPosition, aLatLon);
    setClipDistances(aPosition, aLatLon);
    gl_Position = clip;

    vec2 d = toPixels(projectPoint(aAheadPosition, aAheadLatLon)) - toPixels(clip);
    if (dot(d, d) < 1e-12) return;
    d = normalize(d);
    float arrowLength = aSize * uSlot;
    float angle = aCorner < 0.5 ? 0.0 : (aCorner < 1.5 ? PI - SPREAD : PI + SPREAD);
    float radius = aCorner < 0.5 ? arrowLength : 0.7 * arrowLength;
    vec2 dir = vec2(d.x * cos(angle) - d.y * sin(angle), d.x * sin(angle) + d.y * cos(angle));
    gl_Position = offsetPixels(clip, dir * radius);
}
)";

    const char* FRAGMENT_SHADER = R"(
#version 330 core
in vec4 vColor;
uniform int uRoundPoints; // Points are drawn as discs
out vec4 fragColor;
void main()
{
    if (uRoundPoints != 0 && length(gl_PointCoord - vec2(0.5)) > 0.5) discard;
    fragColor = vColor;
}
)";

    unsigned int compileWithProjection(const char* vertexMain)
    {
        const std::string source = std::string("#version 330 core\n") + PROJECTION_GLSL + vertexMain;
        return compileProgram(source.c_str(), FRAGMENT_SHADER);
    }

    constexpr float DEBUG_LIFT = 1.001f;
    const sf::Color WIREFRAME_COLOR(0, 0, 0, 90);

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
    std::vector<glm::vec3> rims;
    std::vector<int> rimStart(sphere.cells.size() + 1, 0);
    for (std::size_t i = 0; i < sphere.cells.size(); i++) {
        rimStart[i] = static_cast<int>(rims.size());
        for (int t : sphere.cells[i].vertex) rims.push_back(sphere.voronoi_points[t]);
    }
    rimStart[sphere.cells.size()] = static_cast<int>(rims.size());
    return buildPlanetMesh(sphere, rims, rimStart);
}

PlanetMesh buildPlanetMesh(const vor::SphereVoronoi& sphere, const std::vector<glm::vec3>& rims, const std::vector<int>& rimStart)
{
    PlanetMesh mesh;
    const int n = static_cast<int>(sphere.cells.size());
    if (n == 0 || rimStart.size() != sphere.cells.size() + 1) return mesh;

    // Degrees of longitude the rim of cell i turns through, about 360 for the cell holding a pole
    // and 0 for any other
    const auto rimTurn = [&](int i) {
        const int first = rimStart[i], size = rimStart[i + 1] - first;
        if (size == 0) return 0.0;
        const double startLon = latLonOf(rims[first]).y;
        double lon = startLon;
        for (int k = 1; k <= size; k++) lon = mapproj::unwrapLon(latLonOf(rims[first + k % size]).y, lon);
        return lon - startLon;
    };
    // The pole lies in the cell with the nearest centre, which is what findCell returns, unless
    // the smoothing moved the boundary across it. Then it is in one of that cell's neighbours
    const auto poleCell = [&](const glm::vec3& pole, int start) {
        const int nearest = sphere.findCell(pole, start);
        if (std::abs(rimTurn(nearest)) > 180.0) return nearest;
        for (int nb : sphere.cells[nearest].neighbors) {
            if (std::abs(rimTurn(nb)) > 180.0) return nb;
        }
        return nearest;
    };
    const int north = poleCell(glm::vec3(0.f, 1.f, 0.f), 0);
    int south = poleCell(glm::vec3(0.f, -1.f, 0.f), north);
    if (south == north) south = -1; // Only with a handful of cells

    // Centre then rim for every cell, fanned into triangles that stay counter clockwise
    std::vector<unsigned int> poleFans, otherFans;
    mesh.cellFirstVertex.resize(n + 1);
    double lonMin = 0.0, lonMax = 0.0;
    for (int i = 0; i < n; i++) {
        const bool pole = i == north || i == south;
        const int rimFirst = rimStart[i], rimSize = rimStart[i + 1] - rimFirst;
        const unsigned int centre = static_cast<unsigned int>(mesh.positions.size());
        mesh.cellFirstVertex[i] = static_cast<int>(centre);

        const glm::dvec2 centreLL = latLonOf(sphere.points[i]);
        mesh.positions.push_back(sphere.points[i]);
        mesh.latLon.push_back(toRadians(centreLL.x, centreLL.y));
        for (int k = 0; k < rimSize; k++) {
            const glm::vec3& p = rims[rimFirst + k];
            const glm::dvec2 ll = latLonOf(p);
            const double lon = mapproj::unwrapLon(ll.y, centreLL.y);
            mesh.positions.push_back(p);
            mesh.latLon.push_back(toRadians(ll.x, lon));
            if (!pole) {
                lonMin = std::min(lonMin, lon);
                lonMax = std::max(lonMax, lon);
            }
        }

        std::vector<unsigned int>& fans = pole ? poleFans : otherFans;
        const unsigned int size = static_cast<unsigned int>(rimSize);
        for (unsigned int k = 0; k < size; k++) {
            fans.push_back(centre);
            fans.push_back(centre + 1 + k);
            fans.push_back(centre + 1 + (k + 1) % size);
        }
    }
    mesh.cellFirstVertex[n] = static_cast<int>(mesh.positions.size());

    // Pole bands. Walk the rim with each step's longitude unwrapped from the step before; a
    // rim edge never spans 180 degrees of longitude unless it runs through the pole, so the
    // walk ends exactly one turn from where it began. Each rim point gets a partner straight
    // above it on the pole, and every edge becomes the quad between the two pairs
    std::vector<unsigned int> bands;
    const auto addBand = [&](int cell, double poleLat) {
        const int rimFirst = rimStart[cell];
        const int size = rimStart[cell + 1] - rimFirst;
        const unsigned int first = static_cast<unsigned int>(mesh.positions.size());
        const glm::vec3 polePoint(0.f, poleLat > 0.0 ? 1.f : -1.f, 0.f);

        double lon = 0.0;
        for (int k = 0; k <= size; k++) {
            const glm::vec3& p = rims[rimFirst + k % size];
            const glm::dvec2 ll = latLonOf(p);
            lon = k == 0 ? ll.y : mapproj::unwrapLon(ll.y, lon);
            mesh.positions.push_back(p);
            mesh.latLon.push_back(toRadians(ll.x, lon));
            mesh.positions.push_back(polePoint);
            mesh.latLon.push_back(toRadians(poleLat, lon));
            lonMin = std::min(lonMin, lon);
            lonMax = std::max(lonMax, lon);
        }
        const double turn = lon - latLonOf(rims[rimFirst]).y;
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

void appendCellOutline(RibbonBatch& batch, const vor::SphereVoronoi& sphere, int cell, const OverlayStyle& style)
{
    if (cell < 0 || cell >= static_cast<int>(sphere.cells.size())) return;
    std::vector<glm::vec3> ring;
    for (int t : sphere.cells[cell].vertex) ring.push_back(sphere.voronoi_points[t]);
    // A pole cell's ring goes round the pole, the ribbon's padding takes care of that
    batch.addLine(ring, true, style.color, style.widthPx);
}

std::vector<std::vector<glm::vec3>> riverPaths(const vor::SphereVoronoi& sphere, const std::vector<River>& rivers)
{
    std::vector<std::vector<glm::vec3>> paths;
    paths.reserve(rivers.size());
    for (const River& river : rivers) {
        std::vector<glm::vec3> path;
        for (std::size_t cell : river.getPathCells()) {
            if (cell < sphere.points.size()) path.push_back(sphere.points[cell]);
        }
        paths.push_back(std::move(path));
    }
    return paths;
}

void appendLatitudeCircle(RibbonBatch& batch, float lat, const OverlayStyle& style)
{
    std::vector<glm::vec3> circle;
    circle.reserve(360);
    for (int lon = -180; lon < 180; lon++) circle.push_back(latLonToUnit(lat, static_cast<float>(lon)));
    batch.addLine(circle, true, style.color, style.widthPx);
}

bool flatCopies(const PlanetCamera& camera, const glm::vec2& viewport, float lonMin, float lonMax, FlatCopies& out)
{
    // Every copy whose longitude range [lonMin, lonMax] + k turns reaches into the half width
    // either side of the centre
    const float turn = glm::two_pi<float>();
    const float pi = glm::pi<float>();
    const float halfWidth = 0.5f * viewport.x / camera.flatScale();
    const float centreX = camera.flatCentre().x;
    out.first = static_cast<int>(std::ceil((centreX - halfWidth - lonMax) / turn));
    out.last = static_cast<int>(std::floor((centreX + halfWidth - lonMin) / turn));
    const bool wraps = camera.flatWraps();
    if (!wraps) {
        // Only the copies reaching into 180W..180E, so the cells across the 180th meridian fill its edges
        out.first = std::max(out.first, static_cast<int>(std::ceil((-pi - lonMax) / turn)));
        out.last = std::min(out.last, static_cast<int>(std::floor((pi - lonMin) / turn)));
    }
    if (out.last < out.first) return false;
    out.maxX = wraps ? turn + std::max(std::abs(lonMin + out.first * turn), std::abs(lonMax + out.last * turn)) : pi;
    return true;
}

void appendDebugLines(std::vector<DebugVertex>& out, const std::vector<glm::vec3>& pairs, sf::Color color)
{
    for (std::size_t i = 0; i + 1 < pairs.size(); i += 2) {
        const glm::dvec2 a = latLonOf(pairs[i]);
        const glm::dvec2 b = latLonOf(pairs[i + 1]);
        out.push_back({ pairs[i], toRadians(a.x, a.y), color });
        out.push_back({ pairs[i + 1], toRadians(b.x, mapproj::unwrapLon(b.y, a.y)), color });
    }
}

void appendDebugPoints(std::vector<DebugVertex>& out, const std::vector<glm::vec3>& points, sf::Color color, float size)
{
    for (const glm::vec3& p : points) {
        const glm::dvec2 ll = latLonOf(p);
        out.push_back({ p, toRadians(ll.x, ll.y), color, size });
    }
}

void GlobeRenderer::ProjectionUniforms::fetch(unsigned int p)
{
    projection = glGetUniformLocation(p, "uProjection");
    viewProj = glGetUniformLocation(p, "uViewProj");
    lift = glGetUniformLocation(p, "uLift");
    firstCopy = glGetUniformLocation(p, "uFirstCopy");
    maxY = glGetUniformLocation(p, "uMaxY");
    maxX = glGetUniformLocation(p, "uMaxX");
    eye = glGetUniformLocation(p, "uEye");
    horizonClip = glGetUniformLocation(p, "uHorizonClip");
    roundPoints = glGetUniformLocation(p, "uRoundPoints");
}

void GlobeRenderer::ProjectionUniforms::set(int projectionMode, const glm::mat4& viewProjMatrix, float liftValue, const glm::vec3& eyePosition,
    bool clipHorizon, float firstCopyValue, float maxYValue, float maxXValue) const
{
    glUniform1i(projection, projectionMode);
    glUniformMatrix4fv(viewProj, 1, GL_FALSE, glm::value_ptr(viewProjMatrix));
    glUniform1f(lift, liftValue);
    glUniform3fv(eye, 1, glm::value_ptr(eyePosition));
    glUniform1i(horizonClip, clipHorizon ? 1 : 0);
    glUniform1f(firstCopy, firstCopyValue);
    glUniform1f(maxY, maxYValue);
    glUniform1f(maxX, maxXValue);
}

GlobeRenderer::~GlobeRenderer()
{
    release();
    if (contextActive()) {
        if (program) glDeleteProgram(program);
        if (ribbonProgram) glDeleteProgram(ribbonProgram);
        if (arrowProgram) glDeleteProgram(arrowProgram);
    }
}

bool GlobeRenderer::contextActive() const
{
    return contextId != 0 && sf::Context::getActiveContextId() == contextId;
}

void GlobeRenderer::release()
{
    if (contextActive()) {
        std::vector<GLuint> arrays = { vao, debugLineVao, debugPointVao };
        std::vector<GLuint> buffers = { positionBuffer, latLonBuffer, colorBuffer, indexBuffer, debugLineBuffer, debugPointBuffer };
        for (const RibbonLayer& layer : ribbons) {
            arrays.insert(arrays.end(), { layer.vao, layer.discVao });
            buffers.insert(buffers.end(), { layer.buffer, layer.indexBuffer, layer.discBuffer });
        }
        for (const ArrowLayer& layer : arrows) {
            arrays.push_back(layer.vao);
            buffers.push_back(layer.buffer);
        }
        for (GLuint a : arrays) {
            if (a) glDeleteVertexArrays(1, &a);
        }
        for (GLuint b : buffers) {
            if (b) glDeleteBuffers(1, &b);
        }
    }
    vao = debugLineVao = debugPointVao = 0;
    positionBuffer = latLonBuffer = colorBuffer = indexBuffer = debugLineBuffer = debugPointBuffer = 0;
    indexCount = poleFanIndexCount = globeIndexCount = debugLineCount = debugPointCount = 0;
    for (RibbonLayer& layer : ribbons) layer = RibbonLayer();
    for (ArrowLayer& layer : arrows) layer = ArrowLayer();
    cellFirstVertex.clear();
    poleBands.clear();
}

bool GlobeRenderer::ensurePrograms()
{
    const unsigned long long active = sf::Context::getActiveContextId();
    if (active == 0) return false;
    if (active != contextId) program = ribbonProgram = arrowProgram = 0; // Made in another context, which can't use them here
    contextId = active;

    if (!program) {
        program = compileWithProjection(MESH_VERTEX_MAIN);
        if (!program) return false;
        meshUniforms.fetch(program);
    }
    if (!ribbonProgram) {
        ribbonProgram = compileWithProjection(RIBBON_VERTEX_MAIN);
        if (!ribbonProgram) return false;
        ribbonUniforms.fetch(ribbonProgram);
        ribbonViewportLocation = glGetUniformLocation(ribbonProgram, "uViewport");
        ribbonDiscsLocation = glGetUniformLocation(ribbonProgram, "uDiscs");
    }
    if (!arrowProgram) {
        arrowProgram = compileWithProjection(ARROW_VERTEX_MAIN);
        if (!arrowProgram) return false;
        arrowUniforms.fetch(arrowProgram);
        arrowViewportLocation = glGetUniformLocation(arrowProgram, "uViewport");
        arrowSlotLocation = glGetUniformLocation(arrowProgram, "uSlot");
    }
    return true;
}

void GlobeRenderer::build(const vor::SphereVoronoi& sphere)
{
    upload(buildPlanetMesh(sphere));
}

void GlobeRenderer::build(const vor::SphereVoronoi& sphere, const std::vector<glm::vec3>& rims, const std::vector<int>& rimStart)
{
    upload(buildPlanetMesh(sphere, rims, rimStart));
}

void GlobeRenderer::upload(const PlanetMesh& mesh)
{
    release();
    if (!ensurePrograms()) return;

    if (mesh.indices.empty()) return;
    cellFirstVertex = mesh.cellFirstVertex;
    poleBands = mesh.poleBands;
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

    // Debug lines and points, every attribute interleaved
    const auto debugArray = [](GLuint& array, GLuint& buffer) {
        glGenVertexArrays(1, &array);
        glBindVertexArray(array);
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), reinterpret_cast<const void*>(offsetof(DebugVertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DebugVertex), reinterpret_cast<const void*>(offsetof(DebugVertex, color)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), reinterpret_cast<const void*>(offsetof(DebugVertex, latLon)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), reinterpret_cast<const void*>(offsetof(DebugVertex, size)));
    };
    debugArray(debugLineVao, debugLineBuffer);
    debugArray(debugPointVao, debugPointBuffer);

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

void GlobeRenderer::setRibbons(RibbonGroup group, const RibbonBatch& batch)
{
    if (!ready() || !contextActive()) return;
    RibbonLayer& layer = ribbons[static_cast<int>(group)];

    if (!layer.vao) {
        const std::size_t stride = sizeof(RibbonVertex);
        const auto attribute = [stride](GLuint location, GLint size, GLenum type, GLboolean normalised, std::size_t offset) {
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(location, size, type, normalised, static_cast<GLsizei>(stride), reinterpret_cast<const void*>(offset));
        };

        // Lines: vertex v reads the previous point at v, itself at v + 2 and the next point at v + 4
        glGenVertexArrays(1, &layer.vao);
        glBindVertexArray(layer.vao);
        glGenBuffers(1, &layer.buffer);
        glBindBuffer(GL_ARRAY_BUFFER, layer.buffer);
        glGenBuffers(1, &layer.indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, layer.indexBuffer);
        for (int slot = 0; slot < 3; slot++) {
            const std::size_t base = 2 * slot * stride;
            attribute(2 * slot, 3, GL_FLOAT, GL_FALSE, base + offsetof(RibbonVertex, position));
            attribute(2 * slot + 1, 2, GL_FLOAT, GL_FALSE, base + offsetof(RibbonVertex, latLon));
        }
        attribute(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, 2 * stride + offsetof(RibbonVertex, color));
        attribute(7, 1, GL_FLOAT, GL_FALSE, 2 * stride + offsetof(RibbonVertex, halfWidth));
        attribute(8, 1, GL_FLOAT, GL_FALSE, 2 * stride + offsetof(RibbonVertex, side));

        // Discs only have the current point
        glGenVertexArrays(1, &layer.discVao);
        glBindVertexArray(layer.discVao);
        glGenBuffers(1, &layer.discBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, layer.discBuffer);
        attribute(2, 3, GL_FLOAT, GL_FALSE, offsetof(RibbonVertex, position));
        attribute(3, 2, GL_FLOAT, GL_FALSE, offsetof(RibbonVertex, latLon));
        attribute(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(RibbonVertex, color));
        attribute(7, 1, GL_FLOAT, GL_FALSE, offsetof(RibbonVertex, halfWidth));
    }

    // The index buffer belongs to the VAO, so it is only bound with the VAO bound
    glBindVertexArray(layer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, layer.buffer);
    glBufferData(GL_ARRAY_BUFFER, batch.vertices.size() * sizeof(RibbonVertex), batch.vertices.empty() ? nullptr : batch.vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, layer.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, batch.indices.size() * sizeof(GLuint), batch.indices.empty() ? nullptr : batch.indices.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, layer.discBuffer);
    glBufferData(GL_ARRAY_BUFFER, batch.discs.size() * sizeof(RibbonVertex), batch.discs.empty() ? nullptr : batch.discs.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    layer.indexCount = static_cast<int>(batch.indices.size());
    layer.discCount = static_cast<int>(batch.discs.size());
    layer.lonMin = batch.lonMin;
    layer.lonMax = batch.lonMax;
}

void GlobeRenderer::setRibbonsVisible(RibbonGroup group, bool visible)
{
    ribbons[static_cast<int>(group)].visible = visible;
}

void GlobeRenderer::setArrows(ArrowGroup group, const ArrowBatch& batch)
{
    if (!ready() || !contextActive()) return;
    ArrowLayer& layer = arrows[static_cast<int>(group)];

    if (!layer.vao) {
        const auto attribute = [](GLuint location, GLint size, GLenum type, GLboolean normalised, std::size_t offset) {
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(location, size, type, normalised, sizeof(ArrowVertex), reinterpret_cast<const void*>(offset));
        };
        glGenVertexArrays(1, &layer.vao);
        glBindVertexArray(layer.vao);
        glGenBuffers(1, &layer.buffer);
        glBindBuffer(GL_ARRAY_BUFFER, layer.buffer);
        attribute(0, 3, GL_FLOAT, GL_FALSE, offsetof(ArrowVertex, position));
        attribute(1, 2, GL_FLOAT, GL_FALSE, offsetof(ArrowVertex, latLon));
        attribute(2, 3, GL_FLOAT, GL_FALSE, offsetof(ArrowVertex, aheadPosition));
        attribute(3, 2, GL_FLOAT, GL_FALSE, offsetof(ArrowVertex, aheadLatLon));
        attribute(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(ArrowVertex, color));
        attribute(5, 1, GL_FLOAT, GL_FALSE, offsetof(ArrowVertex, size));
        attribute(6, 1, GL_FLOAT, GL_FALSE, offsetof(ArrowVertex, corner));
        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, layer.buffer);
    glBufferData(GL_ARRAY_BUFFER, batch.vertices.size() * sizeof(ArrowVertex), batch.vertices.empty() ? nullptr : batch.vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    layer.vertexCount = static_cast<int>(batch.vertices.size());
    layer.lonMin = batch.lonMin;
    layer.lonMax = batch.lonMax;
}

void GlobeRenderer::setArrowsVisible(ArrowGroup group, bool visible)
{
    arrows[static_cast<int>(group)].visible = visible;
}

void GlobeRenderer::setDebugBuffer(unsigned int buffer, int& count, const std::vector<DebugVertex>& vertices)
{
    count = 0;
    if (!ready()) return;
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(DebugVertex), vertices.empty() ? nullptr : vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    count = static_cast<int>(vertices.size());
}

void GlobeRenderer::setDebugLines(const std::vector<DebugVertex>& lines)
{
    setDebugBuffer(debugLineBuffer, debugLineCount, lines);
}

void GlobeRenderer::setDebugPoints(const std::vector<DebugVertex>& points)
{
    setDebugBuffer(debugPointBuffer, debugPointCount, points);
}

void GlobeRenderer::draw(const PlanetCamera& camera, const glm::vec2& viewport)
{
    if (!ready() || viewport.x <= 0.f || viewport.y <= 0.f) return;

    glViewport(0, 0, static_cast<GLsizei>(viewport.x), static_cast<GLsizei>(viewport.y));
    // Same blending as SFML's BlendAlpha, the height colours fade towards the background with their alpha
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    // Every program writes all four clip distances, the globe's are 1 unless it clips at the horizon
    const GLenum clipPlanes[] = { GL_CLIP_DISTANCE0, GL_CLIP_DISTANCE1, GL_CLIP_DISTANCE2, GL_CLIP_DISTANCE3 };
    for (GLenum plane : clipPlanes) glEnable(plane);

    if (camera.mode == ViewMode::Flat) drawFlat(camera, viewport);
    else drawGlobe(camera, viewport);

    for (GLenum plane : clipPlanes) glDisable(plane);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GlobeRenderer::drawGlobe(const PlanetCamera& camera, const glm::vec2& viewport)
{
    const glm::mat4 viewProj = camera.globeViewProj(viewport);
    const glm::vec3 eye = camera.globeEye(viewport);

    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glUseProgram(program);
    meshUniforms.set(0, viewProj, 1.f, eye, false, 0.f, 0.f, 0.f);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, globeIndexCount, GL_UNSIGNED_INT, nullptr);

    drawRibbons(camera, viewport, RibbonGroup::Rivers, RibbonGroup::FoundCells);
    drawArrows(camera, viewport);

    // The debug overlays are lifted and depth tested against the cells, the wireframe culled like them
    glUseProgram(program);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    meshUniforms.set(0, viewProj, DEBUG_LIFT, eye, false, 0.f, 0.f, 0.f);
    drawOverlays(false, 1);

    drawRibbons(camera, viewport, RibbonGroup::FoundCells, RibbonGroup::Count);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void GlobeRenderer::drawFlat(const PlanetCamera& camera, const glm::vec2& viewport)
{
    FlatCopies copies;
    if (!flatCopies(camera, viewport, lonMin, lonMax, copies)) return;
    const float maxY = mapproj::project(camera.projection, mapproj::maxLat(camera.projection), 0.f).y;

    // Flat cells don't overlap and keep their winding, so no depth test and no culling
    glUseProgram(program);
    meshUniforms.set(static_cast<int>(camera.projection), camera.flatViewProj(viewport), 1.f, glm::vec3(0.f), false,
        static_cast<float>(copies.first), maxY, copies.maxX);
    glBindVertexArray(vao);
    const std::size_t offset = static_cast<std::size_t>(poleFanIndexCount) * sizeof(GLuint);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount - poleFanIndexCount, GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(offset), copies.count());

    drawRibbons(camera, viewport, RibbonGroup::Rivers, RibbonGroup::FoundCells);
    drawArrows(camera, viewport);

    // The debug overlays lie within the mesh's longitude range, so the same copies cover them.
    // The mesh program keeps its uniforms
    glUseProgram(program);
    drawOverlays(true, copies.count());

    drawRibbons(camera, viewport, RibbonGroup::FoundCells, RibbonGroup::Count);
}

void GlobeRenderer::drawRibbons(const PlanetCamera& camera, const glm::vec2& viewport, RibbonGroup from, RibbonGroup to)
{
    // On the surface, so no depth test; the globe clips them at the horizon instead
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glUseProgram(ribbonProgram);
    glUniform2f(ribbonViewportLocation, viewport.x, viewport.y);

    const bool flat = camera.mode == ViewMode::Flat;
    const float maxY = mapproj::project(camera.projection, mapproj::maxLat(camera.projection), 0.f).y;
    if (!flat) ribbonUniforms.set(0, camera.globeViewProj(viewport), 1.f, camera.globeEye(viewport), true, 0.f, 0.f, 0.f);

    for (int g = static_cast<int>(from); g < static_cast<int>(to); g++) {
        const RibbonLayer& layer = ribbons[g];
        if (!layer.visible || (layer.indexCount == 0 && layer.discCount == 0)) continue;

        // Each layer has its own longitude range, so its own copies
        int instances = 1;
        if (flat) {
            FlatCopies copies;
            if (!flatCopies(camera, viewport, layer.lonMin, layer.lonMax, copies)) continue;
            ribbonUniforms.set(static_cast<int>(camera.projection), camera.flatViewProj(viewport), 1.f, glm::vec3(0.f), false,
                static_cast<float>(copies.first), maxY, copies.maxX);
            instances = copies.count();
        }

        if (layer.indexCount > 0) {
            glUniform1i(ribbonDiscsLocation, 0);
            glBindVertexArray(layer.vao);
            glDrawElementsInstanced(GL_TRIANGLES, layer.indexCount, GL_UNSIGNED_INT, nullptr, instances);
        }
        if (layer.discCount > 0) {
            glEnable(GL_PROGRAM_POINT_SIZE);
            glUniform1i(ribbonDiscsLocation, 1);
            glUniform1i(ribbonUniforms.roundPoints, 1);
            glBindVertexArray(layer.discVao);
            glDrawArraysInstanced(GL_POINTS, 0, layer.discCount, instances);
            glUniform1i(ribbonUniforms.roundPoints, 0);
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }
}

void GlobeRenderer::drawArrows(const PlanetCamera& camera, const glm::vec2& viewport)
{
    // Like the ribbons: on the surface, no depth test, clipped at the horizon on the globe. All
    // three corners share the anchor's clip distance, so an arrow is drawn whole or not at all
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glUseProgram(arrowProgram);
    glUniform2f(arrowViewportLocation, viewport.x, viewport.y);
    glUniform1f(arrowSlotLocation, arrowSlotPx);

    const bool flat = camera.mode == ViewMode::Flat;
    const float maxY = mapproj::project(camera.projection, mapproj::maxLat(camera.projection), 0.f).y;
    if (!flat) arrowUniforms.set(0, camera.globeViewProj(viewport), 1.f, camera.globeEye(viewport), true, 0.f, 0.f, 0.f);

    for (const ArrowLayer& layer : arrows) {
        if (!layer.visible || layer.vertexCount == 0) continue;
        int instances = 1;
        if (flat) {
            FlatCopies copies;
            if (!flatCopies(camera, viewport, layer.lonMin, layer.lonMax, copies)) continue;
            arrowUniforms.set(static_cast<int>(camera.projection), camera.flatViewProj(viewport), 1.f, glm::vec3(0.f), false,
                static_cast<float>(copies.first), maxY, copies.maxX);
            instances = copies.count();
        }
        glBindVertexArray(layer.vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, layer.vertexCount, instances);
    }
}
void GlobeRenderer::drawOverlays(bool flat, int instances)
{
    // The globe draws all cell fans, the flat view the pole bands instead of the pole fans
    if (wireframe) {
        const int first = flat ? poleFanIndexCount : 0;
        const int count = flat ? indexCount - poleFanIndexCount : globeIndexCount;
        glBindVertexArray(vao);
        glDisableVertexAttribArray(1);
        glVertexAttrib4f(1, WIREFRAME_COLOR.r / 255.f, WIREFRAME_COLOR.g / 255.f, WIREFRAME_COLOR.b / 255.f, WIREFRAME_COLOR.a / 255.f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_INT,
            reinterpret_cast<const void*>(static_cast<std::size_t>(first) * sizeof(GLuint)), instances);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnableVertexAttribArray(1);
    }

    if (debugLineCount > 0) {
        glBindVertexArray(debugLineVao);
        glDrawArraysInstanced(GL_LINES, 0, debugLineCount, instances);
    }

    if (debugPointCount > 0) {
        glEnable(GL_PROGRAM_POINT_SIZE);
        glUniform1i(meshUniforms.roundPoints, 1);
        glBindVertexArray(debugPointVao);
        glDrawArraysInstanced(GL_POINTS, 0, debugPointCount, instances);
        glUniform1i(meshUniforms.roundPoints, 0);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
}
