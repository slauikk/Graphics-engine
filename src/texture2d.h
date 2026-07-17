#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include <glad/glad.h>
#include <cstdint>
#include <string>

class Texture2D {
public:
    GLuint id;
    int width;
    int height;
    int channels;
    
    Texture2D(const std::string& path, bool flipY = true);
    Texture2D();
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;
    
    void loadFromFile(const std::string& path, bool flipY = true);
    void loadGeneratedGrid();
    void bind(uint32_t slot = 0) const;
    
private:
    void createMagentaFallback();
    void createGridTexture();
};

#endif // TEXTURE2D_H
