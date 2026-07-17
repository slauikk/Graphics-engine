#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <memory>
#include <vector>
#include "vertex_array.h"
#include "vertex_buffer.h"
#include "index_buffer.h"

class Mesh {
public:
    Mesh();
    ~Mesh() = default;

    void setVertexBuffer(std::unique_ptr<VertexBuffer> vbo);
    void setIndexBuffer(std::unique_ptr<IndexBuffer> ebo);
    void setVertexArray(VertexArray&& vao);
    void addVertexBuffer(std::unique_ptr<VertexBuffer> vbo, const BufferLayout& layout,
                         GLuint firstAttribute, GLuint divisor = 0);
    void setVertexCount(GLsizei count) { m_vertexCount = count; }
    void setIndexCount(GLsizei count) { m_indexCount = count; }
    void setPrimitive(GLenum primitive) { m_primitive = primitive; }

    void draw() const;
    void drawInstanced(GLsizei instanceCount) const;

    const VertexArray& getVertexArray() const { return m_vao; }

private:
    VertexArray m_vao;
    std::unique_ptr<VertexBuffer> m_vbo;
    std::unique_ptr<IndexBuffer> m_ebo;
    std::vector<std::unique_ptr<VertexBuffer>> m_additionalVbos;
    GLsizei m_vertexCount = 0;
    GLsizei m_indexCount = 0;
    GLenum m_primitive = GL_TRIANGLES;
};

#endif // MESH_H
