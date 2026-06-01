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

    // Shadow mapping
    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f); // world -> light-clip (для основного прохода)
    unsigned int shadowMap = 0;                   // depth-текстура карты теней
    bool hasShadows = false;                      // доступна ли карта теней в этом проходе
    bool shadowPass = false;                      // идёт ли сейчас depth-pass из света
    bool receiveShadows = true;                   // затеняется ли текущий узел (из Pivot3D)
};