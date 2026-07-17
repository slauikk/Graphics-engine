#include "vertex_array.h"

VertexArray::VertexArray() {
    glGenVertexArrays(1, &m_id);
}

VertexArray::~VertexArray() {
    if (m_id != 0) {
        glDeleteVertexArrays(1, &m_id);
    }
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_id(other.m_id), m_hasIndexBuffer(other.m_hasIndexBuffer) {
    other.m_id = 0;
    other.m_hasIndexBuffer = false;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        if (m_id != 0) {
            glDeleteVertexArrays(1, &m_id);
        }
        m_id = other.m_id;
        m_hasIndexBuffer = other.m_hasIndexBuffer;
        other.m_id = 0;
        other.m_hasIndexBuffer = false;
    }
    return *this;
}

void VertexArray::bind() const {
    glBindVertexArray(m_id);
}

void VertexArray::unbind() const {
    glBindVertexArray(0);
}

void VertexArray::addVertexBuffer(VertexBuffer& vbo, const BufferLayout& layout,
                                  GLuint firstAttribute, GLuint divisor) {
    bind();
    vbo.bind();

    const auto& elements = layout.getElements();
    GLuint index = firstAttribute;
    for (const auto& element : elements) {
        glEnableVertexAttribArray(index);
        if (BufferElement::isIntegerType(element.type)) {
            glVertexAttribIPointer(
                index,
                BufferElement::getComponentCount(element.type),
                BufferElement::getGLType(element.type),
                layout.getStride(),
                reinterpret_cast<const void*>(element.offset)
            );
        } else {
            glVertexAttribPointer(
                index,
                BufferElement::getComponentCount(element.type),
                BufferElement::getGLType(element.type),
                element.normalized ? GL_TRUE : GL_FALSE,
                layout.getStride(),
                reinterpret_cast<const void*>(element.offset)
            );
        }
        glVertexAttribDivisor(index, divisor);
        index++;
    }

    vbo.unbind();
    unbind();
}

void VertexArray::setIndexBuffer(IndexBuffer& ebo) {
    bind();
    ebo.bind();
    m_hasIndexBuffer = true;
    unbind();
}
