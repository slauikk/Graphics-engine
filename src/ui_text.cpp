#include "ui_text.h"
#include "font_data.h"
#include "shader.h"
#include "core/resource_manager.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

namespace {

class ScopedTextRenderState {
public:
    ScopedTextRenderState()
        : m_depthTestEnabled(glIsEnabled(GL_DEPTH_TEST)),
          m_blendEnabled(glIsEnabled(GL_BLEND)) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &m_program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertexArray);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &m_arrayBuffer);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &m_activeTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_texture2D);
        glGetIntegerv(GL_BLEND_SRC_RGB, &m_blendSrcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &m_blendDstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &m_blendSrcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &m_blendDstAlpha);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    ~ScopedTextRenderState() {
        glUseProgram(static_cast<GLuint>(m_program));
        glBindVertexArray(static_cast<GLuint>(m_vertexArray));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(m_arrayBuffer));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_texture2D));
        glActiveTexture(static_cast<GLenum>(m_activeTexture));
        glBlendFuncSeparate(m_blendSrcRgb, m_blendDstRgb, m_blendSrcAlpha, m_blendDstAlpha);

        if (m_blendEnabled) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }

        if (m_depthTestEnabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }

private:
    GLboolean m_depthTestEnabled;
    GLboolean m_blendEnabled;
    GLint m_program = 0;
    GLint m_vertexArray = 0;
    GLint m_arrayBuffer = 0;
    GLint m_activeTexture = GL_TEXTURE0;
    GLint m_texture2D = 0;
    GLint m_blendSrcRgb = GL_ONE;
    GLint m_blendDstRgb = GL_ZERO;
    GLint m_blendSrcAlpha = GL_ONE;
    GLint m_blendDstAlpha = GL_ZERO;
};

} // namespace

GLuint UIText::VAO = 0;
GLuint UIText::VBO = 0;
GLuint UIText::fontTexture = 0;
std::shared_ptr<Shader> UIText::shader = nullptr;
bool UIText::initialized = false;
bool UIText::frameBatchActive = false;
int UIText::windowWidth = 1280;
int UIText::windowHeight = 720;
std::vector<float> UIText::frameVertices;

bool UIText::createTextShader() {
    shader = ResourceManager::getShader("shaders/ui_text.vert", "shaders/ui_text.frag");
    return shader && shader->m_id != 0;
}

bool UIText::createFontAtlas() {
    constexpr int glyphWidth = 8;
    constexpr int glyphHeight = 12;
    constexpr int glyphsPerRow = 16;
    constexpr int atlasWidth = glyphWidth * glyphsPerRow;
    constexpr int atlasHeight = glyphHeight * glyphsPerRow;

    std::vector<unsigned char> atlas(static_cast<std::size_t>(atlasWidth * atlasHeight), 0);
    for (const auto& [character, bitmap] : fontData) {
        const unsigned int code = static_cast<unsigned char>(character);
        const int atlasX = static_cast<int>(code % glyphsPerRow) * glyphWidth;
        const int atlasY = static_cast<int>(code / glyphsPerRow) * glyphHeight;

        for (int row = 0; row < glyphHeight; ++row) {
            for (int sourceColumn = 0; sourceColumn < glyphWidth; ++sourceColumn) {
                if ((bitmap[row] & (0x80 >> sourceColumn)) == 0) {
                    continue;
                }

                const int screenColumn = glyphWidth - 1 - sourceColumn;
                atlas[static_cast<std::size_t>((atlasY + row) * atlasWidth + atlasX + screenColumn)] = 255;
            }
        }
    }

    GLint previousTexture = 0;
    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);

    glGenTextures(1, &fontTexture);
    if (fontTexture == 0) {
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasWidth, atlasHeight, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    return true;
}

void UIText::init(int windowWidth, int windowHeight) {
    if (initialized) return;
    
    UIText::windowWidth = windowWidth;
    UIText::windowHeight = windowHeight;
    
    if (!createTextShader()) {
        std::cerr << "[UIText] Failed to create shader\n";
        return;
    }
    initFontData();
    if (!createFontAtlas()) {
        std::cerr << "[UIText] Failed to create font atlas\n";
        shader.reset();
        return;
    }
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(4 * sizeof(float)));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    initialized = true;
}

void UIText::beginFrame() {
    if (!initialized) {
        return;
    }

    frameVertices.clear();
    frameBatchActive = true;
}

void UIText::flush() {
    if (!frameBatchActive) {
        return;
    }

    frameBatchActive = false;
    drawBatch(frameVertices);
    frameVertices.clear();
}

void UIText::updateWindowSize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
}

void UIText::appendCharVertices(std::vector<float>& vertices, char c, float x, float y,
                                float scale, float r, float g, float b) {
    if (fontData.find(c) == fontData.end()) {
        c = ' ';
    }

    constexpr float glyphsPerRow = 16.0f;
    const unsigned int code = static_cast<unsigned char>(c);
    const float atlasColumn = static_cast<float>(code % 16);
    const float atlasRow = static_cast<float>(code / 16);
    const float u0 = atlasColumn / glyphsPerRow;
    const float v0 = atlasRow / glyphsPerRow;
    const float u1 = (atlasColumn + 1.0f) / glyphsPerRow;
    const float v1 = (atlasRow + 1.0f) / glyphsPerRow;

    const float x1 = x + 8.0f * scale;
    const float y1 = y + 12.0f * scale;
    const float quad[] = {
        x,  y,  u0, v0,
        x1, y,  u1, v0,
        x1, y1, u1, v1,
        x,  y,  u0, v0,
        x1, y1, u1, v1,
        x,  y1, u0, v1
    };

    for (int vertex = 0; vertex < 6; ++vertex) {
        const int offset = vertex * 4;
        vertices.push_back(quad[offset]);
        vertices.push_back(quad[offset + 1]);
        vertices.push_back(quad[offset + 2]);
        vertices.push_back(quad[offset + 3]);
        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
    }
}

void UIText::appendTextVertices(std::vector<float>& vertices, const std::string& text,
                                float x, float y, float scale, float offsetX, float offsetY,
                                float r, float g, float b) {
    const float charWidth = 8.0f * scale;
    float currentX = x;
    float currentY = y;

    for (char c : text) {
        if (c == '\n') {
            currentY += 14.0f * scale;
            currentX = x;
            continue;
        }

        appendCharVertices(vertices, c, currentX + offsetX, currentY + offsetY, scale, r, g, b);
        currentX += charWidth;
    }
}

void UIText::drawBatch(const std::vector<float>& vertices) {
    if (!initialized || shader == nullptr || shader->m_id == 0 || vertices.empty()) {
        return;
    }

    ScopedTextRenderState renderState;
    shader->use();

    const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth),
                                            static_cast<float>(windowHeight), 0.0f);
    shader->setMat4("projection", projection);
    shader->setInt("fontAtlas", 0);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 7));
}

void UIText::renderTextInternal(const std::string& text, float x, float y, float scale,
                                float r, float g, float b) {
    if (!initialized || shader == nullptr || shader->m_id == 0 || text.empty()) {
        return;
    }

    constexpr std::size_t floatsPerGlyphQuad = 42;
    std::vector<float> immediateVertices;
    std::vector<float>& vertices = frameBatchActive ? frameVertices : immediateVertices;
    vertices.reserve(vertices.size() + text.size() * 5 * floatsPerGlyphQuad);

    const float outlineOffset = 1.5f * scale;
    appendTextVertices(vertices, text, x, y, scale, -outlineOffset, 0.0f, 0.0f, 0.0f, 0.0f);
    appendTextVertices(vertices, text, x, y, scale, outlineOffset, 0.0f, 0.0f, 0.0f, 0.0f);
    appendTextVertices(vertices, text, x, y, scale, 0.0f, -outlineOffset, 0.0f, 0.0f, 0.0f);
    appendTextVertices(vertices, text, x, y, scale, 0.0f, outlineOffset, 0.0f, 0.0f, 0.0f);
    appendTextVertices(vertices, text, x, y, scale, 0.0f, 0.0f, r, g, b);

    if (!frameBatchActive) {
        drawBatch(immediateVertices);
    }
}

void UIText::renderText(const std::string& text, float x, float y, float scale) {
    renderTextInternal(text, x, y, scale, 1.0f, 1.0f, 1.0f);
}

void UIText::renderTextWithColor(const std::string& text, float x, float y, float scale,
                                 float r, float g, float b) {
    renderTextInternal(text, x, y, scale, r, g, b);
}

void UIText::cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (fontTexture != 0) {
        glDeleteTextures(1, &fontTexture);
        fontTexture = 0;
    }
    shader.reset();
    frameVertices.clear();
    frameVertices.shrink_to_fit();
    frameBatchActive = false;
    initialized = false;
}

bool UIText::reloadShaders() {
    if (!initialized) return false;
    ResourceManager::reloadAllShaders();
    return shader && shader->m_id != 0;
}
