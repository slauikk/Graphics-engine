#include "index_buffer.h"

IndexBuffer::IndexBuffer(const uint32_t* indices, uint32_t count)
    : m_count(count) {
    GLint previousArrayBuffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);

    glGenBuffers(1, &m_id);
    if (m_id == 0) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
}

IndexBuffer::~IndexBuffer() {
    if (m_id != 0) {
        glDeleteBuffers(1, &m_id);
    }
}

void IndexBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
}

void IndexBuffer::unbind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
