#ifndef UI_TEXT_H
#define UI_TEXT_H

#include <glad/glad.h>
#include <memory>
#include <string>
#include <vector>

class Shader;

class UIText {
public:
    static void init(int windowWidth, int height);
    static void beginFrame();
    static void flush();
    static void renderText(const std::string& text, float x, float y, float scale = 1.0f);
    static void renderTextWithColor(const std::string& text, float x, float y, float scale, float r, float g, float b);
    static void cleanup();
    static void updateWindowSize(int width, int height);
    static bool reloadShaders();
    
private:
    static GLuint VAO, VBO, fontTexture;
    static std::shared_ptr<Shader> shader;
    static bool initialized;
    static bool frameBatchActive;
    static int windowWidth, windowHeight;
    static std::vector<float> frameVertices;
    
    static bool createTextShader();
    static bool createFontAtlas();
    static void appendCharVertices(std::vector<float>& vertices, char c, float x, float y,
                                   float scale, float r, float g, float b);
    static void appendTextVertices(std::vector<float>& vertices, const std::string& text,
                                   float x, float y, float scale, float offsetX, float offsetY,
                                   float r, float g, float b);
    static void drawBatch(const std::vector<float>& vertices);
    static void renderTextInternal(const std::string& text, float x, float y, float scale,
                                   float r, float g, float b);
};

#endif // UI_TEXT_H
