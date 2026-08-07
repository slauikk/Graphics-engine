#ifndef MATH_GEOMETRY_H
#define MATH_GEOMETRY_H

#include <glm/glm.hpp>

namespace geometry {

struct AxisAlignedBounds {
    glm::vec3 minimum{0.0f};
    glm::vec3 maximum{0.0f};
    bool valid = false;
};

} // namespace geometry

#endif // MATH_GEOMETRY_H
