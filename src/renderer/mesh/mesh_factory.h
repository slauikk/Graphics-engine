#ifndef MESH_FACTORY_H
#define MESH_FACTORY_H

#include "mesh.h"
#include <cstdint>
#include <memory>
#include <vector>

class MeshFactory {
public:
    // Creates a cube with indexed rendering (24 unique vertices, 36 indices)
    // Format: pos(3), normal(3), uv(2) per vertex
    static std::shared_ptr<Mesh> CreateCube();
    static std::shared_ptr<Mesh> CreateInterleaved(
        const std::vector<float>& vertices,
        const std::vector<std::uint32_t>& indices);
};

#endif // MESH_FACTORY_H
