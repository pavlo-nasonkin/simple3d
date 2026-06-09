#include "FlyCameraBehaviour.h"

#include <algorithm>

// GL тут не нужен — только константы клавиш/кнопок GLFW. GLFW_INCLUDE_NONE не даёт
// glfw3.h подтянуть gl.h (иначе требовался бы glew.h строго раньше).
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Pivot3D.h"
#include "Engine.h"
#include "input/MouseInput.h"
#include "input/KeyboardInput.h"

void FlyCameraBehaviour::OnStart()
{
    // Начальные yaw/pitch берём из текущего поворота узла.
    if (Pivot3D* owner = GetOwner()) {
        const glm::vec3* rotation = owner->GetRotation();
        _pitch = rotation->x;
        _yaw = rotation->y;
    }
}

void FlyCameraBehaviour::OnUpdate(float deltaTime)
{
    Pivot3D* owner = GetOwner();
    if (!owner) {
        return;
    }

    const auto& mouse = Engine::GetInstance().GetMouseInput();
    // Навигация только при зажатой ПКМ (иначе ввод в UI двигал бы камеру).
    if (!mouse->IsButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        _hasLastMouse = false;
        return;
    }

    // ── Обзор мышью ──
    const double mouseX = mouse->GetMouseX();
    const double mouseY = mouse->GetMouseY();
    if (_hasLastMouse) {
        const float dx = static_cast<float>(mouseX - _lastX);
        const float dy = static_cast<float>(mouseY - _lastY);
        _yaw   -= dx * _lookSensitivity;
        _pitch -= dy * _lookSensitivity;
        const float limit = glm::radians(89.0f);
        _pitch = std::clamp(_pitch, -limit, limit);
    }
    _lastX = mouseX;
    _lastY = mouseY;
    _hasLastMouse = true;

    owner->SetRotation(_pitch, _yaw, 0.0f);

    // ── Перемещение WASD/QE ──
    // Базис из тех же углов, что и поворот узла (Rx*Ry), чтобы движение совпадало со взглядом.
    const glm::mat4 rot = glm::rotate(glm::mat4(1.0f), _pitch, glm::vec3(1.0f, 0.0f, 0.0f))
                        * glm::rotate(glm::mat4(1.0f), _yaw,   glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 forward = glm::vec3(rot * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    // Right — горизонтальный (из forward×up), чтобы стрейф не уезжал вверх/вниз (без roll).
    glm::vec3 right = glm::cross(forward, worldUp);
    right = (glm::dot(right, right) > 1e-6f) ? glm::normalize(right) : glm::vec3(1.0f, 0.0f, 0.0f);

    const auto& keyboard = Engine::GetInstance().GetKeyboardInput();
    glm::vec3 move(0.0f);
    if (keyboard->IsKeyPressed(GLFW_KEY_W)) move += forward;
    if (keyboard->IsKeyPressed(GLFW_KEY_S)) move -= forward;
    if (keyboard->IsKeyPressed(GLFW_KEY_D)) move += right;
    if (keyboard->IsKeyPressed(GLFW_KEY_A)) move -= right;
    if (keyboard->IsKeyPressed(GLFW_KEY_E)) move += worldUp;
    if (keyboard->IsKeyPressed(GLFW_KEY_Q)) move -= worldUp;

    if (glm::dot(move, move) > 0.0f) {
        move = glm::normalize(move) * (_moveSpeed * deltaTime);
        owner->Translate(move.x, move.y, move.z);
    }
}
