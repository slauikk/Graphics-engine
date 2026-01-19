#include "ui_text.h"
#include "font_data.h"
#include "shader.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

GLuint UIText::VAO = 0;
GLuint UIText::VBO = 0;
Shader* UIText::shader = nullptr;
bool UIText::initialized = false;
int UIText::windowWidth = 1280;
int UIText::windowHeight = 720;

bool UIText::createTextShader() {
    if (shader != nullptr) {
        delete shader;
    }
    
    const char* vertexSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
)";

    const char* fragmentSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 textColor;

void main() {
    FragColor = vec4(textColor, 1.0);
}
)";
    
    shader = new Shader("assets/shaders/ui_text.vert", "assets/shaders/ui_text.frag");
    if (shader->m_id == 0) {
        std::cerr << "[UIText] Shader files not found, using inline fallback\n";
        delete shader;
        shader = new Shader(vertexSource, fragmentSource);
    }
    return shader->m_id != 0;
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 2, nullptr, GL_DYNAMIC_DRAW);
    
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

void UIText::renderChar(char c, float x, float y, float scale, bool isOutline) {
    if (fontData.find(c) == fontData.end()) {
        c = ' ';
    }
    
    unsigned char* bitmap = fontData[c];
    float charWidth = 8.0f * scale;
    float charHeight = 12.0f * scale;
    float pixelSize = scale;
    
    for (int row = 0; row < 12; row++) {
        unsigned char rowData = bitmap[row];
        for (int col = 0; col < 8; col++) {
            if (rowData & (0x80 >> col)) {
                float px = x + (7 - col) * pixelSize;
                float py = y + row * pixelSize;
                
                float vertices[6][2] = {
                    { px, py },
                    { px + pixelSize, py },
                    { px + pixelSize, py + pixelSize },
                    { px, py },
                    { px + pixelSize, py + pixelSize },
                    { px, py + pixelSize }
                };
                
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }
}

void UIText::renderText(const std::string& text, float x, float y, float scale) {
    if (!initialized || shader == nullptr) return;
    
    shader->use();
    
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth),
                                     static_cast<float>(windowHeight), 0.0f);
    shader->setMat4("projection", projection);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float charWidth = 8.0f * scale;
    float currentX = x;
    float currentY = y;
    
    shader->setVec3("textColor", 0.0f, 0.0f, 0.0f);
    
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == '\n') {
            currentY += 14.0f * scale;
            currentX = x;
            continue;
        }
        
        float outlineOffset = 1.5f * scale;
        renderChar(text[i], currentX - outlineOffset, currentY, scale, true);
        renderChar(text[i], currentX + outlineOffset, currentY, scale, true);
        renderChar(text[i], currentX, currentY - outlineOffset, scale, true);
        renderChar(text[i], currentX, currentY + outlineOffset, scale, true);
        
        currentX += charWidth;
    }
    
    currentX = x;
    currentY = y;
    shader->setVec3("textColor", 1.0f, 1.0f, 1.0f);
    
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == '\n') {
            currentY += 14.0f * scale;
            currentX = x;
            continue;
        }
        
        renderChar(text[i], currentX, currentY, scale, false);
        currentX += charWidth;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

void UIText::renderTextWithColor(const std::string& text, float x, float y, float scale, float r, float g, float b) {
    if (!initialized || shader == nullptr) return;
    
    shader->use();
    
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth),
                                     static_cast<float>(windowHeight), 0.0f);
    shader->setMat4("projection", projection);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float charWidth = 8.0f * scale;
    float currentX = x;
    float currentY = y;
    
    shader->setVec3("textColor", 0.0f, 0.0f, 0.0f);
    float outlineOffset = 1.5f * scale;
    
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == '\n') {
            currentY += 14.0f * scale;
            currentX = x;
            continue;
        }
        
        renderChar(text[i], currentX - outlineOffset, currentY, scale, true);
        renderChar(text[i], currentX + outlineOffset, currentY, scale, true);
        renderChar(text[i], currentX, currentY - outlineOffset, scale, true);
        renderChar(text[i], currentX, currentY + outlineOffset, scale, true);
        
        currentX += charWidth;
    }
    
    currentX = x;
    currentY = y;
    shader->setVec3("textColor", r, g, b);
    
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == '\n') {
            currentY += 14.0f * scale;
            currentX = x;
            continue;
        }
        
        renderChar(text[i], currentX, currentY, scale, false);
        currentX += charWidth;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
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
    if (shader != nullptr) {
        delete shader;
        shader = nullptr;
    }
    initialized = false;
}

bool UIText::reloadShaders() {
    if (!initialized) return false;
    return createTextShader();
}
