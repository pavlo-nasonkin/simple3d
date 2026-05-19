#pragma once
#include <glm/glm.hpp>

class Scene3D;
class Camera;

struct RenderContext {
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);            // текущая world-матрица узла
    Camera* camera = nullptr;
    Scene3D* scene3D = nullptr;
};