#include "framebuffer.h"
#include <cmath>

namespace {

class ScopedFramebufferInitBindings {
public:
    ScopedFramebufferInitBindings() {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_drawFramebuffer);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_readFramebuffer);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_texture2D);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &m_renderbuffer);
    }

    ~ScopedFramebufferInitBindings() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(m_drawFramebuffer));
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(m_readFramebuffer));
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_texture2D));
        glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(m_renderbuffer));
    }

private:
    GLint m_drawFramebuffer = 0;
    GLint m_readFramebuffer = 0;
    GLint m_texture2D = 0;
    GLint m_renderbuffer = 0;
};

} // namespace

Framebuffer::~Framebuffer() {
    release();
}

bool Framebuffer::init(GLsizei width, GLsizei height) {
    release();
    const ScopedFramebufferInitBindings restoreBindings;

    if (width <= 0 || height <= 0) {
        return false;
    }

    glGenFramebuffers(1, &m_id);
    if (m_id == 0) {
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_id);

    glGenTextures(1, &m_colorTexture);
    if (m_colorTexture == 0) {
        release();
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_colorTexture, 0);

    constexpr GLenum colorAttachment = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &colorAttachment);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    glGenRenderbuffers(1, &m_depthStencilRenderbuffer);
    if (m_depthStencilRenderbuffer == 0) {
        release();
        return false;
    }
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, m_depthStencilRenderbuffer);

    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

    if (!complete) {
        release();
        return false;
    }

    m_width = width;
    m_height = height;
    return true;
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_id);
}

void Framebuffer::bindDefault() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::blitColorToDefault(GLsizei destinationWidth, GLsizei destinationHeight) const {
    if (!isValid() || destinationWidth <= 0 || destinationHeight <= 0) {
        return;
    }

    const float sourceAspect = static_cast<float>(m_width) / static_cast<float>(m_height);
    const float destinationAspect = static_cast<float>(destinationWidth) /
                                    static_cast<float>(destinationHeight);

    GLsizei blitWidth = destinationWidth;
    GLsizei blitHeight = destinationHeight;
    if (destinationAspect > sourceAspect) {
        blitWidth = static_cast<GLsizei>(std::lround(destinationHeight * sourceAspect));
    } else {
        blitHeight = static_cast<GLsizei>(std::lround(destinationWidth / sourceAspect));
    }

    const GLint x = (destinationWidth - blitWidth) / 2;
    const GLint y = (destinationHeight - blitHeight) / 2;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, m_width, m_height,
                      x, y, x + blitWidth, y + blitHeight,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    bindDefault();
}

void Framebuffer::release() {
    if (m_depthStencilRenderbuffer != 0) {
        glDeleteRenderbuffers(1, &m_depthStencilRenderbuffer);
        m_depthStencilRenderbuffer = 0;
    }
    if (m_colorTexture != 0) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_id != 0) {
        glDeleteFramebuffers(1, &m_id);
        m_id = 0;
    }
    m_width = 0;
    m_height = 0;
}
