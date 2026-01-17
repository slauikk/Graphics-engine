#include "ui_text.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <sstream>
#include <iomanip>
#include <map>

GLuint UIText::VAO = 0;
GLuint UIText::VBO = 0;
GLuint UIText::shaderProgram = 0;
bool UIText::initialized = false;
int UIText::windowWidth = 1280;
int UIText::windowHeight = 720;


static std::map<char, unsigned char[12]> fontData;

static void initFont() {
    // '0'
    fontData['0'][0] = 0x3E; fontData['0'][1] = 0x63; fontData['0'][2] = 0x73; fontData['0'][3] = 0x7B;
    fontData['0'][4] = 0x6F; fontData['0'][5] = 0x67; fontData['0'][6] = 0x63; fontData['0'][7] = 0x63;
    fontData['0'][8] = 0x63; fontData['0'][9] = 0x63; fontData['0'][10] = 0x63; fontData['0'][11] = 0x3E;
    
    // '1'
    fontData['1'][0] = 0x18; fontData['1'][1] = 0x1C; fontData['1'][2] = 0x18; fontData['1'][3] = 0x18;
    fontData['1'][4] = 0x18; fontData['1'][5] = 0x18; fontData['1'][6] = 0x18; fontData['1'][7] = 0x18;
    fontData['1'][8] = 0x18; fontData['1'][9] = 0x18; fontData['1'][10] = 0x18; fontData['1'][11] = 0x7E;
    
    // '2'
    fontData['2'][0] = 0x3E; fontData['2'][1] = 0x63; fontData['2'][2] = 0x60; fontData['2'][3] = 0x60;
    fontData['2'][4] = 0x30; fontData['2'][5] = 0x18; fontData['2'][6] = 0x0C; fontData['2'][7] = 0x06;
    fontData['2'][8] = 0x03; fontData['2'][9] = 0x03; fontData['2'][10] = 0x63; fontData['2'][11] = 0x7F;
    
    // '3'
    fontData['3'][0] = 0x3E; fontData['3'][1] = 0x63; fontData['3'][2] = 0x60; fontData['3'][3] = 0x60;
    fontData['3'][4] = 0x3C; fontData['3'][5] = 0x60; fontData['3'][6] = 0x60; fontData['3'][7] = 0x60;
    fontData['3'][8] = 0x60; fontData['3'][9] = 0x60; fontData['3'][10] = 0x63; fontData['3'][11] = 0x3E;
    
    // '4'
    fontData['4'][0] = 0x30; fontData['4'][1] = 0x38; fontData['4'][2] = 0x3C; fontData['4'][3] = 0x36;
    fontData['4'][4] = 0x33; fontData['4'][5] = 0x31; fontData['4'][6] = 0x7F; fontData['4'][7] = 0x30;
    fontData['4'][8] = 0x30; fontData['4'][9] = 0x30; fontData['4'][10] = 0x30; fontData['4'][11] = 0x30;
    
    // '5'
    fontData['5'][0] = 0x7F; fontData['5'][1] = 0x03; fontData['5'][2] = 0x03; fontData['5'][3] = 0x03;
    fontData['5'][4] = 0x3F; fontData['5'][5] = 0x60; fontData['5'][6] = 0x60; fontData['5'][7] = 0x60;
    fontData['5'][8] = 0x60; fontData['5'][9] = 0x60; fontData['5'][10] = 0x63; fontData['5'][11] = 0x3E;
    
    // '6'
    fontData['6'][0] = 0x3E; fontData['6'][1] = 0x63; fontData['6'][2] = 0x03; fontData['6'][3] = 0x03;
    fontData['6'][4] = 0x3F; fontData['6'][5] = 0x63; fontData['6'][6] = 0x63; fontData['6'][7] = 0x63;
    fontData['6'][8] = 0x63; fontData['6'][9] = 0x63; fontData['6'][10] = 0x63; fontData['6'][11] = 0x3E;
    
    // '7'
    fontData['7'][0] = 0x7F; fontData['7'][1] = 0x63; fontData['7'][2] = 0x60; fontData['7'][3] = 0x60;
    fontData['7'][4] = 0x30; fontData['7'][5] = 0x30; fontData['7'][6] = 0x18; fontData['7'][7] = 0x18;
    fontData['7'][8] = 0x0C; fontData['7'][9] = 0x0C; fontData['7'][10] = 0x0C; fontData['7'][11] = 0x0C;
    
    // '8'
    fontData['8'][0] = 0x3E; fontData['8'][1] = 0x63; fontData['8'][2] = 0x63; fontData['8'][3] = 0x63;
    fontData['8'][4] = 0x3E; fontData['8'][5] = 0x63; fontData['8'][6] = 0x63; fontData['8'][7] = 0x63;
    fontData['8'][8] = 0x63; fontData['8'][9] = 0x63; fontData['8'][10] = 0x63; fontData['8'][11] = 0x3E;
    
    // '9'
    fontData['9'][0] = 0x3E; fontData['9'][1] = 0x63; fontData['9'][2] = 0x63; fontData['9'][3] = 0x63;
    fontData['9'][4] = 0x63; fontData['9'][5] = 0x7E; fontData['9'][6] = 0x60; fontData['9'][7] = 0x60;
    fontData['9'][8] = 0x60; fontData['9'][9] = 0x60; fontData['9'][10] = 0x63; fontData['9'][11] = 0x3E;
    
    // 'F'
    fontData['F'][0] = 0x7F; fontData['F'][1] = 0x03; fontData['F'][2] = 0x03; fontData['F'][3] = 0x03;
    fontData['F'][4] = 0x3F; fontData['F'][5] = 0x03; fontData['F'][6] = 0x03; fontData['F'][7] = 0x03;
    fontData['F'][8] = 0x03; fontData['F'][9] = 0x03; fontData['F'][10] = 0x03; fontData['F'][11] = 0x03;
    
    // 'P'
    fontData['P'][0] = 0x3F; fontData['P'][1] = 0x63; fontData['P'][2] = 0x63; fontData['P'][3] = 0x63;
    fontData['P'][4] = 0x63; fontData['P'][5] = 0x3F; fontData['P'][6] = 0x03; fontData['P'][7] = 0x03;
    fontData['P'][8] = 0x03; fontData['P'][9] = 0x03; fontData['P'][10] = 0x03; fontData['P'][11] = 0x03;
    
    // 'S'
    fontData['S'][0] = 0x3E; fontData['S'][1] = 0x63; fontData['S'][2] = 0x03; fontData['S'][3] = 0x03;
    fontData['S'][4] = 0x0E; fontData['S'][5] = 0x38; fontData['S'][6] = 0x60; fontData['S'][7] = 0x60;
    fontData['S'][8] = 0x60; fontData['S'][9] = 0x60; fontData['S'][10] = 0x63; fontData['S'][11] = 0x3E;
    
    // 'O'
    fontData['O'][0] = 0x3E; fontData['O'][1] = 0x63; fontData['O'][2] = 0x63; fontData['O'][3] = 0x63;
    fontData['O'][4] = 0x63; fontData['O'][5] = 0x63; fontData['O'][6] = 0x63; fontData['O'][7] = 0x63;
    fontData['O'][8] = 0x63; fontData['O'][9] = 0x63; fontData['O'][10] = 0x63; fontData['O'][11] = 0x3E;
    
    // 'V'
    fontData['V'][0] = 0x63; fontData['V'][1] = 0x63; fontData['V'][2] = 0x63; fontData['V'][3] = 0x63;
    fontData['V'][4] = 0x63; fontData['V'][5] = 0x63; fontData['V'][6] = 0x63; fontData['V'][7] = 0x36;
    fontData['V'][8] = 0x36; fontData['V'][9] = 0x1C; fontData['V'][10] = 0x1C; fontData['V'][11] = 0x08;
    
    // ':'
    fontData[':'][0] = 0x00; fontData[':'][1] = 0x00; fontData[':'][2] = 0x18; fontData[':'][3] = 0x18;
    fontData[':'][4] = 0x00; fontData[':'][5] = 0x00; fontData[':'][6] = 0x00; fontData[':'][7] = 0x00;
    fontData[':'][8] = 0x18; fontData[':'][9] = 0x18; fontData[':'][10] = 0x00; fontData[':'][11] = 0x00;
    
    // '.'
    fontData['.'][0] = 0x00; fontData['.'][1] = 0x00; fontData['.'][2] = 0x00; fontData['.'][3] = 0x00;
    fontData['.'][4] = 0x00; fontData['.'][5] = 0x00; fontData['.'][6] = 0x00; fontData['.'][7] = 0x00;
    fontData['.'][8] = 0x00; fontData['.'][9] = 0x00; fontData['.'][10] = 0x18; fontData['.'][11] = 0x18;
    
    // ' '
    for (int i = 0; i < 12; i++) {
        fontData[' '][i] = 0x00;
    }
}

GLuint UIText::createTextShader() {
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

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

void UIText::init(int windowWidth, int windowHeight) {
    if (initialized) return;
    
    UIText::windowWidth = windowWidth;
    UIText::windowHeight = windowHeight;
    
    shaderProgram = createTextShader();
    initFont();
    
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
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }
}

void UIText::renderText(const std::string& text, float x, float y, float scale) {
    if (!initialized) return;
    
    glUseProgram(shaderProgram);
    
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth),
                                     static_cast<float>(windowHeight), 0.0f);
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    glBindVertexArray(VAO);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float charWidth = 8.0f * scale;
    float currentX = x;
    float currentY = y;
    
    GLint colorLoc = glGetUniformLocation(shaderProgram, "textColor");
    glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);
    
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == '\n') {
            currentY -= 14.0f * scale;
            currentX = x;
            continue;
        }
        
        float outlineOffset = 1.5f * scale;
        renderChar(text[i], currentX - outlineOffset, currentY, scale, true);
        renderChar(text[i], currentX + outlineOffset, currentY, scale, true);
        renderChar(text[i], currentX, currentY - outlineOffset, scale, true);
        renderChar(text[i], currentX, currentY + outlineOffset, scale, true);
        renderChar(text[i], currentX - outlineOffset, currentY - outlineOffset, scale, true);
        renderChar(text[i], currentX + outlineOffset, currentY - outlineOffset, scale, true);
        renderChar(text[i], currentX - outlineOffset, currentY + outlineOffset, scale, true);
        renderChar(text[i], currentX + outlineOffset, currentY + outlineOffset, scale, true);
        
        currentX += charWidth;
    }
    
    currentX = x;
    currentY = y;
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
    
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == '\n') {
            currentY -= 14.0f * scale;
            currentX = x;
            continue;
        }
        
        renderChar(text[i], currentX, currentY, scale, false);
        currentX += charWidth;
    }
    
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
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    initialized = false;
}
