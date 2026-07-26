#include "mesh.h"

Mesh::Mesh() = default;

void Mesh::setVertexBuffer(std::unique_ptr<VertexBuffer> vbo) {
    m_vbo = std::move(vbo);
}

void Mesh::setIndexBuffer(std::unique_ptr<IndexBuffer> ebo) {
    if (ebo) {
        m_vao.setIndexBuffer(*ebo);
    }
    m_ebo = std::move(ebo);
}

void Mesh::setVertexArray(VertexArray&& vao) {
    m_vao = std::move(vao);
    if (m_ebo) {
        m_vao.setIndexBuffer(*m_ebo);
    }
}

void Mesh::addVertexBuffer(std::unique_ptr<VertexBuffer> vbo, const BufferLayout& layout,
                           GLuint firstAttribute, GLuint divisor) {
    if (!vbo) {
        return;
    }

    m_vao.addVertexBuffer(*vbo, layout, firstAttribute, divisor);
    m_additionalVbos.push_back(std::move(vbo));
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

void Mesh::drawInstanced(GLsizei instanceCount) const {
    if (instanceCount <= 0) {
        return;
    }

    m_vao.bind();
    if (m_ebo) {
        glDrawElementsInstanced(m_primitive, m_indexCount, GL_UNSIGNED_INT, nullptr, instanceCount);
    } else {
        glDrawArraysInstanced(m_primitive, 0, m_vertexCount, instanceCount);
    }
    m_vao.unbind();
}
