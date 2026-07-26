#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <glad/glad.h>

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    bool init(GLsizei width, GLsizei height);
    void bind() const;
    static void bindDefault();
    void blitColorToDefault(GLsizei destinationWidth, GLsizei destinationHeight) const;

    bool isValid() const { return m_id != 0; }
    GLuint colorTexture() const { return m_colorTexture; }
    GLsizei width() const { return m_width; }
    GLsizei height() const { return m_height; }

private:
    void release();

    GLuint m_id = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthStencilRenderbuffer = 0;
    GLsizei m_width = 0;
    GLsizei m_height = 0;
};

#endif // FRAMEBUFFER_H
