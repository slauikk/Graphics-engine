#include "mesh.h"

Mesh::Mesh() = default;

void Mesh::setVertexBuffer(std::unique_ptr<VertexBuffer> vbo) {
    m_vbo = std::move(vbo);
}

void Mesh::setIndexBuffer(std::unique_ptr<IndexBuffer> ebo) {
    m_ebo = std::move(ebo);
}

void Mesh::setVertexArray(VertexArray&& vao) {
    m_vao = std::move(vao);
}

void Mesh::draw() const {
    m_vao.bind();
    if (m_ebo) {
        glDrawElements(m_primitive, m_indexCount, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(m_primitive, 0, m_vertexCount);
    }
    m_vao.unbind();
}
