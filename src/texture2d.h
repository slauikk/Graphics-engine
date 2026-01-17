#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include <glad/glad.h>
#include <string>

class Texture2D {
public:
    GLuint id;
    int width;
    int height;
    int channels;
    
    Texture2D(const std::string& path, bool flipY = true);
    ~Texture2D();
    
    void bind(uint32_t slot = 0) const;
    
private:
    void createMagentaFallback();
};

#endif // TEXTURE2D_H
