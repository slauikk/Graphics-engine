#include "coordinate_grid.h"

#include "../shader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace {

constexpr std::size_t kFloatsPerVertex = 7;

constexpr const char* kGridVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 u_View;
uniform mat4 u_Projection;

out vec4 vColor;
out vec3 vWorldPosition;

void main() {
    vColor = aColor;
    vWorldPosition = aPosition;
    gl_Position = u_Projection * u_View * vec4(aPosition, 1.0);
}
)";

constexpr const char* kGridFragmentShader = R"(
#version 330 core
in vec4 vColor;
in vec3 vWorldPosition;

uniform vec3 u_CameraPosition;
uniform float u_FadeStart;
uniform float u_FadeEnd;

out vec4 FragColor;

void main() {
    float cameraDistance = length(vWorldPosition.xz - u_CameraPosition.xz);
    float fade = 1.0 - smoothstep(u_FadeStart, u_FadeEnd, cameraDistance);
    float alpha = vColor.a * fade;
    if (alpha < 0.01) {
        discard;
    }
    FragColor = vec4(vColor.rgb, alpha);
}
)";

void appendVertex(std::vector<float>& vertices, const glm::vec3& position,
                  const glm::vec4& color) {
    vertices.insert(vertices.end(), {
        position.x, position.y, position.z,
        color.r, color.g, color.b, color.a
    });
}

void appendLine(std::vector<float>& vertices, const glm::vec3& start,
                const glm::vec3& end, const glm::vec4& color) {
    appendVertex(vertices, start, color);
    appendVertex(vertices, end, color);
}

} // namespace

CoordinateGrid::~CoordinateGrid() {
    if (m_vertexBuffer != 0) {
        glDeleteBuffers(1, &m_vertexBuffer);
    }
    if (m_vertexArray != 0) {
        glDeleteVertexArrays(1, &m_vertexArray);
    }
}

bool CoordinateGrid::init(int halfLineCount, float spacing) {
    if (halfLineCount <= 0 || halfLineCount > 1'000 ||
        !std::isfinite(spacing) || spacing <= 0.0f) {
        return false;
    }

    const std::size_t lineCount = static_cast<std::size_t>(halfLineCount * 2);
    if (lineCount > (std::numeric_limits<std::size_t>::max)() / (4 * kFloatsPerVertex)) {
        return false;
    }

    std::vector<float> vertices;
    vertices.reserve((lineCount * 4 + 6) * kFloatsPerVertex);

    const float extent = static_cast<float>(halfLineCount) * spacing;
    if (!std::isfinite(extent)) {
        return false;
    }
    const glm::vec4 minorColor(0.40f, 0.45f, 0.53f, 0.28f);
    const glm::vec4 majorColor(0.55f, 0.60f, 0.68f, 0.46f);

    for (int line = -halfLineCount; line <= halfLineCount; ++line) {
        if (line == 0) {
            continue;
        }

        const float coordinate = static_cast<float>(line) * spacing;
        const glm::vec4 color = line % 5 == 0 ? majorColor : minorColor;
        appendLine(vertices, {coordinate, 0.0f, -extent},
                   {coordinate, 0.0f, extent}, color);
        appendLine(vertices, {-extent, 0.0f, coordinate},
                   {extent, 0.0f, coordinate}, color);
    }

    m_gridVertexCount = static_cast<GLsizei>(vertices.size() / kFloatsPerVertex);
    appendLine(vertices, {-extent, 0.0f, 0.0f}, {extent, 0.0f, 0.0f},
               {0.95f, 0.24f, 0.22f, 0.95f});
    appendLine(vertices, {0.0f, 0.0f, -extent}, {0.0f, 0.0f, extent},
               {0.25f, 0.48f, 1.0f, 0.95f});
    appendLine(vertices, {0.0f, -extent * 0.25f, 0.0f},
               {0.0f, extent * 0.5f, 0.0f},
               {0.25f, 0.90f, 0.38f, 0.95f});
    m_axisVertexCount = static_cast<GLsizei>(vertices.size() / kFloatsPerVertex) -
                        m_gridVertexCount;

    m_shader = std::make_unique<Shader>(kGridVertexShader, kGridFragmentShader);
    if (m_shader->m_id == 0) {
        m_shader.reset();
        return false;
    }

    GLint previousVertexArray = 0;
    GLint previousArrayBuffer = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);

    glGenVertexArrays(1, &m_vertexArray);
    glGenBuffers(1, &m_vertexBuffer);
    if (m_vertexArray == 0 || m_vertexBuffer == 0) {
        return false;
    }

    glBindVertexArray(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(kFloatsPerVertex * sizeof(float)),
                          nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(kFloatsPerVertex * sizeof(float)),
                          reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
    glBindVertexArray(static_cast<GLuint>(previousVertexArray));

    m_fadeEnd = extent;
    m_fadeStart = extent * 0.55f;
    return true;
}

void CoordinateGrid::draw(const glm::mat4& view, const glm::mat4& projection,
                          const glm::vec3& cameraPosition) const {
    if (!isReady()) {
        return;
    }

    GLint previousProgram = 0;
    GLint previousVertexArray = 0;
    GLint previousBlendSourceRgb = 0;
    GLint previousBlendDestinationRgb = 0;
    GLint previousBlendSourceAlpha = 0;
    GLint previousBlendDestinationAlpha = 0;
    GLint previousBlendEquationRgb = 0;
    GLint previousBlendEquationAlpha = 0;
    GLboolean previousDepthMask = GL_TRUE;
    GLfloat previousLineWidth = 1.0f;
    std::array<GLfloat, 2> lineWidthRange{1.0f, 1.0f};

    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
    glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSourceRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDestinationRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSourceAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDestinationAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &previousBlendEquationRgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &previousBlendEquationAlpha);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange.data());

    const bool blendWasEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;
    const bool depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    m_shader->use();
    m_shader->setMat4("u_View", view);
    m_shader->setMat4("u_Projection", projection);
    m_shader->setVec3("u_CameraPosition", cameraPosition);
    m_shader->setFloat("u_FadeStart", m_fadeStart);
    m_shader->setFloat("u_FadeEnd", m_fadeEnd);

    glBindVertexArray(m_vertexArray);
    glLineWidth(std::clamp(1.0f, lineWidthRange[0], lineWidthRange[1]));
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    glLineWidth(std::clamp(2.0f, lineWidthRange[0], lineWidthRange[1]));
    glDrawArrays(GL_LINES, m_gridVertexCount, m_axisVertexCount);

    glLineWidth(previousLineWidth);
    glDepthMask(previousDepthMask);
    glBlendEquationSeparate(static_cast<GLenum>(previousBlendEquationRgb),
                            static_cast<GLenum>(previousBlendEquationAlpha));
    glBlendFuncSeparate(static_cast<GLenum>(previousBlendSourceRgb),
                        static_cast<GLenum>(previousBlendDestinationRgb),
                        static_cast<GLenum>(previousBlendSourceAlpha),
                        static_cast<GLenum>(previousBlendDestinationAlpha));
    if (!blendWasEnabled) {
        glDisable(GL_BLEND);
    }
    if (!depthTestWasEnabled) {
        glDisable(GL_DEPTH_TEST);
    }
    glUseProgram(static_cast<GLuint>(previousProgram));
    glBindVertexArray(static_cast<GLuint>(previousVertexArray));
}

bool CoordinateGrid::isReady() const {
    return m_vertexArray != 0 && m_vertexBuffer != 0 &&
           m_gridVertexCount > 0 && m_axisVertexCount > 0 &&
           m_shader != nullptr && m_shader->m_id != 0;
}
