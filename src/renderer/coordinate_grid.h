#ifndef COORDINATE_GRID_H
#define COORDINATE_GRID_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>

class Shader;

class CoordinateGrid {
public:
    CoordinateGrid() = default;
    ~CoordinateGrid();

    CoordinateGrid(const CoordinateGrid&) = delete;
    CoordinateGrid& operator=(const CoordinateGrid&) = delete;

    bool init(int halfLineCount = 20, float spacing = 1.0f);
    void draw(const glm::mat4& view, const glm::mat4& projection,
              const glm::vec3& cameraPosition) const;
    bool isReady() const;

private:
    GLuint m_vertexArray = 0;
    GLuint m_vertexBuffer = 0;
    GLsizei m_gridVertexCount = 0;
    GLsizei m_axisVertexCount = 0;
    float m_fadeStart = 0.0f;
    float m_fadeEnd = 0.0f;
    std::unique_ptr<Shader> m_shader;
};

#endif // COORDINATE_GRID_H
