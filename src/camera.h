#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;
    
    float yaw;
    float pitch;
    float fov;
    
    float movementSpeed;
    float mouseSensitivity;
    
    Camera(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f),
           float yaw = -90.0f, float pitch = 0.0f);
    
    glm::mat4 getViewMatrix() const;
    
    void processKeyboard(CameraMovement direction, float deltaTime);
    void processMouse(float xoffset, float yoffset);
    void processScroll(float yoffset);
    
private:
    void updateCameraVectors();
};

#endif // CAMERA_H
