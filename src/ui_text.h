фів#ifndef UI_TEXT_H
#define UI_TEXT_H

#include <glad/glad.h>
#include <string>

class UIText {
public:
    static void init(int windowWidth, int windowHeight);
    static void renderText(const std::string& text, float x, float y, float scale = 1.0f);
    static void cleanup();
    static void updateWindowSize(int width, int height);
    
private:
    static GLuint VAO, VBO;
    static GLuint shaderProgram;
    static bool initialized;
    static int windowWidth, windowHeight;
    
    static GLuint createTextShader();
    static void renderChar(char c, float x, float y, float scale, bool isOutline = false);
};

#endif // UI_TEXT_H
