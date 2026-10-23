#include <glad/glad.h>
#include "GlobeRenderer.hpp"
#include "GLContext.hpp"
#include <SFML/Window/Context.hpp>
#include "Logger.h"
#include <glm/gtc/type_ptr.hpp>

namespace {
    const char* VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
uniform mat4 uViewProj;
uniform float uLift; // Pushes overlays off the surface so they don't z-fight with it
out vec4 vColor;
void main()
{
    vColor = aColor;
    gl_Position = uViewProj * vec4(aPosition * uLift, 1.0);
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
        GLuint buffers[] = { positionBuffer, colorBuffer, indexBuffer, highlightBuffer };
        for (GLuint b : buffers) {
            if (b) glDeleteBuffers(1, &b);
        }
    }
    vao = highlightVao = positionBuffer = colorBuffer = indexBuffer = highlightBuffer = 0;
    indexCount = highlightCount = 0;
    cellFirstVertex.clear();
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
    }

    // Centre then ring for every cell, fanned into triangles that stay counter clockwise
    std::vector<glm::vec3> positions;
    std::vector<GLuint> indices;
    const int n = static_cast<int>(sphere.cells.size());
    cellFirstVertex.resize(n + 1);
    for (int i = 0; i < n; i++) {
        const std::vector<int>& ring = sphere.cells[i].vertex;
        const GLuint centre = static_cast<GLuint>(positions.size());
        cellFirstVertex[i] = static_cast<int>(centre);
        positions.push_back(sphere.points[i]);
        for (int t : ring) positions.push_back(sphere.voronoi_points[t]);

        const GLuint size = static_cast<GLuint>(ring.size());
        for (GLuint k = 0; k < size; k++) {
            indices.push_back(centre);
            indices.push_back(centre + 1 + k);
            indices.push_back(centre + 1 + (k + 1) % size);
        }
    }
    cellFirstVertex[n] = static_cast<int>(positions.size());
    vertexColors.assign(positions.size(), sf::Color(128, 128, 128));

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    // sf::Color is four bytes, r g b a, which is exactly a normalised ubyte4 attribute
    glGenBuffers(1, &colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexColors.size() * sizeof(sf::Color), vertexColors.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(sf::Color), nullptr);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    indexCount = static_cast<int>(indices.size());

    // The highlight ring, only positions; its colour is a constant attribute
    glGenVertexArrays(1, &highlightVao);
    glBindVertexArray(highlightVao);
    glGenBuffers(1, &highlightBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, highlightBuffer);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    LOG_INFO(Graphics, "Globe mesh built, vertices and triangles:", static_cast<int>(positions.size()), indexCount / 3);
}

void GlobeRenderer::setCellColors(const std::vector<sf::Color>& colors)
{
    if (!ready() || colors.size() + 1 != cellFirstVertex.size()) return;

    for (std::size_t i = 0; i < colors.size(); i++) {
        std::fill(vertexColors.begin() + cellFirstVertex[i], vertexColors.begin() + cellFirstVertex[i + 1], colors[i]);
    }
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexColors.size() * sizeof(sf::Color), vertexColors.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlobeRenderer::setHighlight(const vor::SphereVoronoi& sphere, int cell)
{
    if (!ready()) return;
    if (cell < 0 || cell >= static_cast<int>(sphere.cells.size())) {
        highlightCount = 0;
        return;
    }

    std::vector<glm::vec3> ring;
    for (int t : sphere.cells[cell].vertex) ring.push_back(sphere.voronoi_points[t]);
    glBindBuffer(GL_ARRAY_BUFFER, highlightBuffer);
    glBufferData(GL_ARRAY_BUFFER, ring.size() * sizeof(glm::vec3), ring.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    highlightCount = static_cast<int>(ring.size());
}

void GlobeRenderer::draw(const PlanetCamera& camera, const glm::vec2& viewport)
{
    if (!ready() || viewport.x <= 0.f || viewport.y <= 0.f) return;

    const glm::mat4 viewProj = camera.globeViewProj(viewport);

    glViewport(0, 0, static_cast<GLsizei>(viewport.x), static_cast<GLsizei>(viewport.y));
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    // Same blending as SFML's BlendAlpha, the height colours fade towards the background with their alpha
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(program);
    glUniformMatrix4fv(viewProjLocation, 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform1f(liftLocation, 1.f);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

    if (highlightCount > 0) {
        glUniform1f(liftLocation, HIGHLIGHT_LIFT);
        glBindVertexArray(highlightVao);
        glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
        glLineWidth(2.f);
        glDrawArrays(GL_LINE_LOOP, 0, highlightCount);
        glLineWidth(1.f);
    }

    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}
