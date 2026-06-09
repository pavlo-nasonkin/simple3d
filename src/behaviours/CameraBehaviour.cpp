#include "CameraBehaviour.h"

#include <glm/glm.hpp>

#include "camera/Camera.h"
#include "Pivot3D.h"

CameraBehaviour::CameraBehaviour()
    : _camera(std::make_shared<Camera>())
{
}

CameraBehaviour::CameraBehaviour(float fovYDeg, float nearPlane, float farPlane)
    : _camera(std::make_shared<Camera>()),
      _fovYDeg(fovYDeg), _nearPlane(nearPlane), _farPlane(farPlane)
{
}

void CameraBehaviour::SetPerspective(float fovYDeg, float nearPlane, float farPlane)
{
    _fovYDeg = fovYDeg;
    _nearPlane = nearPlane;
    _farPlane = farPlane;
    _camera->buildProjectionMatrix(_fovYDeg, _nearPlane, _farPlane);
}

void CameraBehaviour::SetViewportSize(float width, float height)
{
    if (width <= 0.0f || height <= 0.0f) {
        return;
    }
    _camera->SetScreenWidth(width);
    _camera->SetScreenHeight(height);
    _camera->buildProjectionMatrix(_fovYDeg, _nearPlane, _farPlane);
}

void CameraBehaviour::Refresh()
{
    Pivot3D* owner = GetOwner();
    if (!owner) {
        return;
    }
    const glm::mat4 world = owner->WorldMatrix();
    _camera->Position = glm::vec3(world[3]);
    _camera->Front = glm::normalize(-glm::vec3(world[2])); // forward = −Z узла
    // Up берём мировой (а не из колонки матрицы): иначе при сочетании yaw+pitch
    // базис наклоняется и камера кренится вокруг оси взгляда (roll). lookAt в
    // Camera::GetViewMatrix перестроит ортонормированный базис из Front+worldUp без roll.
    _camera->WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    _camera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
}

void CameraBehaviour::OnAttach()
{
    _camera->buildProjectionMatrix(_fovYDeg, _nearPlane, _farPlane);
    Refresh();
}

void CameraBehaviour::OnUpdate(float /*deltaTime*/)
{
    Refresh();
}
