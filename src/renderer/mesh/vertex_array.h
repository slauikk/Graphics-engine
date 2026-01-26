#ifndef VERTEX_ARRAY_H
#define VERTEX_ARRAY_H

#include <glad/glad.h>
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "vertex_layout.h"

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    
    // Move constructor and assignment
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void bind() const;
    void unbind() const;

    void addVertexBuffer(VertexBuffer& vbo, const BufferLayout& layout);
    void setIndexBuffer(IndexBuffer& ebo);

    GLuint getId() const { return m_id; }
    bool hasIndexBuffer() const { return m_hasIndexBuffer; }

private:
    GLuint m_id = 0;
    bool m_hasIndexBuffer = false;
};

#endif // VERTEX_ARRAY_H
