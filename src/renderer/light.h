#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
    bool enabled;
    
    DirectionalLight()
        : direction(glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f)))
        , color(glm::vec3(1.0f))
        , enabled(true) {}
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    float constant;
    float linear;
    float quadratic;
    bool enabled;
    
    PointLight()
        : position(glm::vec3(2.0f, 2.0f, 2.0f))
        , color(glm::vec3(1.0f))
        , constant(1.0f)
        , linear(0.09f)
        , quadratic(0.032f)
        , enabled(true) {}
};

#endif // LIGHT_H
