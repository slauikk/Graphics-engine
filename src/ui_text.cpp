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
    GLint m_blendSrcRgb = GL_ONE;
    GLint m_blendDstRgb = GL_ZERO;
    GLint m_blendSrcAlpha = GL_ONE;
    GLint m_blendDstAlpha = GL_ZERO;
};

} // namespace

GLuint UIText::VAO = 0;
GLuint UIText::VBO = 0;
std::shared_ptr<Shader> UIText::shader = nullptr;
bool UIText::initialized = false;
int UIText::windowWidth = 1280;
int UIText::windowHeight = 720;

bool UIText::createTextShader() {
    shader = ResourceManager::getShader("shaders/ui_text.vert", "shaders/ui_text.frag");
    return shader && shader->m_id != 0;
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
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    initialized = true;
}

void UIText::updateWindowSize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
}

void UIText::appendCharVertices(std::vector<float>& vertices, char c, float x, float y, float scale) {
    if (fontData.find(c) == fontData.end()) {
        c = ' ';
    }
    
    unsigned char* bitmap = fontData[c];
    float pixelSize = scale;
    
    for (int row = 0; row < 12; row++) {
        unsigned char rowData = bitmap[row];
        for (int col = 0; col < 8; col++) {
            if (rowData & (0x80 >> col)) {
                float px = x + (7 - col) * pixelSize;
                float py = y + row * pixelSize;
                
                const float x1 = px + pixelSize;
                const float y1 = py + pixelSize;
                const float quad[] = {
                    px, py, x1, py, x1, y1,
                    px, py, x1, y1, px, y1
                };
                vertices.insert(vertices.end(), quad, quad + 12);
            }
        }
    }
}

void UIText::appendTextVertices(std::vector<float>& vertices, const std::string& text,
                                float x, float y, float scale, float offsetX, float offsetY) {
    const float charWidth = 8.0f * scale;
    float currentX = x;
    float currentY = y;

    for (char c : text) {
        if (c == '\n') {
            currentY += 14.0f * scale;
            currentX = x;
            continue;
        }

        appendCharVertices(vertices, c, currentX + offsetX, currentY + offsetY, scale);
        currentX += charWidth;
    }
}

void UIText::drawBatch(const std::vector<float>& vertices, float r, float g, float b) {
    if (vertices.empty()) {
        return;
    }

    shader->setVec3("textColor", r, g, b);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 2));
}

void UIText::renderTextInternal(const std::string& text, float x, float y, float scale,
                                float r, float g, float b) {
    if (!initialized || shader == nullptr || shader->m_id == 0 || text.empty()) {
        return;
    }

    ScopedTextRenderState renderState;
    shader->use();

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth),
                                     static_cast<float>(windowHeight), 0.0f);
    shader->setMat4("projection", projection);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    constexpr std::size_t estimatedPixelsPerGlyph = 32;
    std::vector<float> outlineVertices;
    std::vector<float> fillVertices;
    outlineVertices.reserve(text.size() * 4 * estimatedPixelsPerGlyph * 12);
    fillVertices.reserve(text.size() * estimatedPixelsPerGlyph * 12);

    const float outlineOffset = 1.5f * scale;
    appendTextVertices(outlineVertices, text, x, y, scale, -outlineOffset, 0.0f);
    appendTextVertices(outlineVertices, text, x, y, scale, outlineOffset, 0.0f);
    appendTextVertices(outlineVertices, text, x, y, scale, 0.0f, -outlineOffset);
    appendTextVertices(outlineVertices, text, x, y, scale, 0.0f, outlineOffset);
    appendTextVertices(fillVertices, text, x, y, scale, 0.0f, 0.0f);

    drawBatch(outlineVertices, 0.0f, 0.0f, 0.0f);
    drawBatch(fillVertices, r, g, b);
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
    shader.reset();
    initialized = false;
}

bool UIText::reloadShaders() {
    if (!initialized) return false;
    ResourceManager::reloadAllShaders();
    return shader && shader->m_id != 0;
}
