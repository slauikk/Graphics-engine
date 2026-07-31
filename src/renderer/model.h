#ifndef MODEL_H
#define MODEL_H

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Mesh;

struct ModelPart {
    std::shared_ptr<Mesh> mesh;
    std::string materialId;
};

class Model {
public:
    explicit Model(std::vector<ModelPart> parts, std::size_t vertexCount,
                   std::size_t indexCount)
        : m_parts(std::move(parts))
        , m_vertexCount(vertexCount)
        , m_indexCount(indexCount) {}

    const std::vector<ModelPart>& parts() const { return m_parts; }
    std::size_t vertexCount() const { return m_vertexCount; }
    std::size_t indexCount() const { return m_indexCount; }

private:
    std::vector<ModelPart> m_parts;
    std::size_t m_vertexCount = 0;
    std::size_t m_indexCount = 0;
};

#endif // MODEL_H
