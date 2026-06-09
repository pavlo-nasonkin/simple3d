#include "OrbitCameraBehaviour.h"

#include <algorithm>

#define GLFW_INCLUDE_NONE // нужны только константы кнопок GLFW, без gl.h
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Pivot3D.h"
#include "Engine.h"
#include "input/MouseInput.h"
#include "object_selector/ObjectSelector.h"

glm::vec3 OrbitCameraBehaviour::ForwardFromAngles() const
{
    // Тот же базис, что у узла (Rx*Ry) → совпадает с forward, который выведет CameraBehaviour.
    const glm::mat4 rot = glm::rotate(glm::mat4(1.0f), _pitch, glm::vec3(1.0f, 0.0f, 0.0f))
                        * glm::rotate(glm::mat4(1.0f), _yaw,   glm::vec3(0.0f, 1.0f, 0.0f));
    return glm::vec3(rot * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
}

void OrbitCameraBehaviour::ApplyToOwner()
{
    if (Pivot3D* owner = GetOwner()) {
        const glm::vec3 forward = ForwardFromAngles();
        const glm::vec3 pos = _target - forward * _radius; // камера на сфере вокруг фокуса
        owner->SetPosition(pos.x, pos.y, pos.z);
        owner->SetRotation(_pitch, _yaw, 0.0f);
    }
}

void OrbitCameraBehaviour::OnStart()
{
    if (Pivot3D* owner = GetOwner()) {
        const glm::vec3* rotation = owner->GetRotation();
        _pitch = rotation->x;
        _yaw = rotation->y;
        // Точка фокуса — перед камерой на текущем расстоянии.
        _target = *owner->GetPosition() + ForwardFromAngles() * _radius;
    }
}

void OrbitCameraBehaviour::OnUpdate(float /*deltaTime*/)
{
    Pivot3D* owner = GetOwner();
    if (!owner) {
        return;
    }

    const auto& mouse = Engine::GetInstance().GetMouseInput();
    const bool middle = mouse->IsButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE);
    const bool right = mouse->IsButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
    const double mouseX = mouse->GetMouseX();
    const double mouseY = mouse->GetMouseY();
    const double scroll = mouse->ConsumeScrollY();

    // По фронту нажатия ПКМ выбираем точку фокуса: выбранный объект или точка перед камерой.
    if (right && !_prevRight) {
        const glm::vec3 pos = *owner->GetPosition();
        auto selected = Engine::GetInstance().GetObjectSelector()->GetSelectedObject();
        if (selected && selected.get() != owner) {
            _target = *selected->GetPosition();
            _radius = std::max(glm::length(_target - pos), 0.1f);
        } else {
            _target = pos + ForwardFromAngles() * _radius;
        }
    }
    _prevRight = right;

    bool changed = false;

    if ((middle || right) && _hasLastMouse) {
        const float dx = static_cast<float>(mouseX - _lastX);
        const float dy = static_cast<float>(mouseY - _lastY);

        if (right) {
            // Орбита: знаки как в старом FreeLookCamera (вправо → +yaw, вниз → −pitch).
            _yaw += dx * _orbitSpeed;
            _pitch -= dy * _orbitSpeed;
            const float limit = glm::radians(89.0f);
            _pitch = std::clamp(_pitch, -limit, limit);
            changed = true;
        } else if (middle) {
            // Pan: двигаем точку фокуса в плоскости экрана (камера следует за ней).
            const glm::vec3 forward = ForwardFromAngles();
            const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
            const glm::vec3 rightVec = glm::normalize(glm::cross(forward, worldUp));
            const glm::vec3 upVec = glm::normalize(glm::cross(rightVec, forward));
            const float scale = _panSpeed * _radius;
            _target += (-dx * scale) * rightVec + (dy * scale) * upVec;
            changed = true;
        }
    }
    _lastX = mouseX;
    _lastY = mouseY;
    _hasLastMouse = (middle || right);

    if (scroll != 0.0) {
        // Колесо — dolly: приближение к фокусу (как Position += Front*scroll у старой камеры).
        _radius = std::max(_radius - static_cast<float>(scroll) * _dollyStep, 0.1f);
        changed = true;
    }

    if (changed) {
        ApplyToOwner();
    }
}
