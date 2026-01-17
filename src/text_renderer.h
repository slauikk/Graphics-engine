#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <glad/glad.h>
#include <string>

class TextRenderer {
public:
    static void init(int windowWidth, int windowHeight);
    static void renderText(const std::string& text, float x, float y, float scale);
    static void cleanup();
    
private:
    static GLuint VAO, VBO;
    static GLuint shaderProgram;
    static bool initialized;
    
    static GLuint createTextShader();
};

#endif // TEXT_RENDERER_H
