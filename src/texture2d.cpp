#define STB_IMAGE_IMPLEMENTATION
#include "texture2d.h"
#include <stb_image.h>
#include <iostream>

Texture2D::Texture2D(const std::string& path, bool flipY) 
    : id(0), width(0), height(0), channels(0) {
    loadFromFile(path, flipY);
}

Texture2D::Texture2D() 
    : id(0), width(0), height(0), channels(0) {
}

void Texture2D::loadFromFile(const std::string& path, bool flipY) {
    if (id != 0) {
        glDeleteTextures(1, &id);
        id = 0;
    }
    
    stbi_set_flip_vertically_on_load(flipY);
    
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load texture: " << path << "\n";
        createMagentaFallback();
        return;
    }
    
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    
    GLenum format = GL_RGB;
    if (channels == 4) {
        format = GL_RGBA;
    } else if (channels == 1) {
        format = GL_RED;
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(data);
}

void Texture2D::loadGeneratedGrid() {
    if (id != 0) {
        glDeleteTextures(1, &id);
        id = 0;
    }
    
    createGridTexture();
}

Texture2D::~Texture2D() {
    if (id != 0) {
        glDeleteTextures(1, &id);
    }
}

void Texture2D::bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture2D::createMagentaFallback() {
    createGridTexture();
}

void Texture2D::createGridTexture() {
    width = 8;
    height = 8;
    channels = 3;
    
    unsigned char checkerboard[8 * 8 * 3];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int idx = (y * 8 + x) * 3;
            bool isWhite = ((x + y) % 2) == 0;
            if (isWhite) {
                checkerboard[idx] = 255;     // R
                checkerboard[idx + 1] = 255; // G
                checkerboard[idx + 2] = 255; // B
            } else {
                checkerboard[idx] = 255;     // R (magenta)
                checkerboard[idx + 1] = 0;   // G
                checkerboard[idx + 2] = 255; // B
            }
        }
    }
    
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, checkerboard);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
