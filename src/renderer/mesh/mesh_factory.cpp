#include "mesh_factory.h"
#include "mesh.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "vertex_array.h"
#include "vertex_layout.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <limits>

std::shared_ptr<Mesh> MeshFactory::CreateInterleaved(
    const std::vector<float>& vertices,
    const std::vector<std::uint32_t>& indices) {
    constexpr std::size_t floatsPerVertex = 8;
    if (vertices.empty() || indices.empty() || vertices.size() % floatsPerVertex != 0 ||
        vertices.size() / floatsPerVertex >
            static_cast<std::size_t>((std::numeric_limits<GLsizei>::max)()) ||
        indices.size() > static_cast<std::size_t>((std::numeric_limits<GLsizei>::max)()) ||
        indices.size() > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)())) {
        return nullptr;
    }

    auto mesh = std::make_shared<Mesh>();
    auto vbo = std::make_unique<VertexBuffer>(
        vertices.data(), vertices.size() * sizeof(float), GL_STATIC_DRAW);
    auto ebo = std::make_unique<IndexBuffer>(
        indices.data(), static_cast<std::uint32_t>(indices.size()));
    if (vbo->getId() == 0 || ebo->getId() == 0) {
        return nullptr;
    }

    VertexArray vao;
    BufferLayout layout({
        { ShaderDataType::Float3, "aPos" },
        { ShaderDataType::Float3, "aNormal" },
        { ShaderDataType::Float2, "aTexCoord" }
    });
    vao.addVertexBuffer(*vbo, layout);
    vao.setIndexBuffer(*ebo);

    mesh->setVertexBuffer(std::move(vbo));
    mesh->setIndexBuffer(std::move(ebo));
    mesh->setVertexArray(std::move(vao));
    mesh->setVertexCount(static_cast<GLsizei>(vertices.size() / floatsPerVertex));
    mesh->setIndexCount(static_cast<GLsizei>(indices.size()));
    mesh->setPrimitive(GL_TRIANGLES);
    return mesh;
}

std::shared_ptr<Mesh> MeshFactory::CreateCube() {
    // 24 unique vertices (4 per face, with correct normals and UVs)
    // Format: pos(3), normal(3), uv(2) = 8 floats per vertex
    std::vector<float> vertices = {
        // Back face (normal: 0, 0, -1)
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f, // 0
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f, // 1
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // 2
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, // 3
        
        // Front face (normal: 0, 0, 1)
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, // 4
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // 5
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // 6
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // 7
        
        // Left face (normal: -1, 0, 0)
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 8
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 9
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 10
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 11
        
        // Right face (normal: 1, 0, 0)
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 12
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 13
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 14
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 15
        
        // Bottom face (normal: 0, -1, 0)
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f, // 16
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f, // 17
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f, // 18
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f, // 19
        
        // Top face (normal: 0, 1, 0)
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 20
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 21
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 22
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f  // 23
    };
    
    // 36 indices (6 faces * 2 triangles * 3 vertices)
    std::vector<uint32_t> indices = {
        // Back face
        0, 1, 2,  2, 3, 0,
        // Front face
        4, 5, 6,  6, 7, 4,
        // Left face
        8, 9, 10,  10, 11, 8,
        // Right face
        12, 13, 14,  14, 15, 12,
        // Bottom face
        16, 17, 18,  18, 19, 16,
        // Top face
        20, 21, 22,  22, 23, 20
    };
    
    return CreateInterleaved(vertices, indices);
}
