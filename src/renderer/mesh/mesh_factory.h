#ifndef MESH_FACTORY_H
#define MESH_FACTORY_H

#include "mesh.h"
#include <memory>

class MeshFactory {
public:
    // Creates a cube with indexed rendering (24 unique vertices, 36 indices)
    // Format: pos(3), normal(3), uv(2) per vertex
    static std::shared_ptr<Mesh> CreateCube();
};

#endif // MESH_FACTORY_H
