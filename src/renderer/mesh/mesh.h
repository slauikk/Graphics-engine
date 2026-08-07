#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include "math/geometry.h"
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
    void setBounds(const geometry::AxisAlignedBounds& bounds) { m_bounds = bounds; }
    void setPickingGeometry(
        std::vector<glm::vec3> positions,
        std::vector<std::uint32_t> indices) {
        m_pickingPositions = std::move(positions);
        m_pickingIndices = std::move(indices);
    }

    void draw() const;
    void drawInstanced(GLsizei instanceCount) const;

    const VertexArray& getVertexArray() const { return m_vao; }
    const geometry::AxisAlignedBounds& bounds() const { return m_bounds; }
    const std::vector<glm::vec3>& pickingPositions() const { return m_pickingPositions; }
    const std::vector<std::uint32_t>& pickingIndices() const { return m_pickingIndices; }

private:
    VertexArray m_vao;
    std::unique_ptr<VertexBuffer> m_vbo;
    std::unique_ptr<IndexBuffer> m_ebo;
    std::vector<std::unique_ptr<VertexBuffer>> m_additionalVbos;
    GLsizei m_vertexCount = 0;
    GLsizei m_indexCount = 0;
    GLenum m_primitive = GL_TRIANGLES;
    geometry::AxisAlignedBounds m_bounds;
    std::vector<glm::vec3> m_pickingPositions;
    std::vector<std::uint32_t> m_pickingIndices;
};

#endif // MESH_H
