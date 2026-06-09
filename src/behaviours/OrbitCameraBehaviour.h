#pragma once

#include <string>

#include <glm/glm.hpp>

#include "Behaviour.h"

// Орбитальная навигация (паритет со старым FreeLookCamera, REFACTORING_PLAN 7.8):
//   СКМ-drag  — pan (сдвиг точки фокуса в плоскости экрана),
//   ПКМ-drag  — орбита вокруг точки фокуса (выбранный объект или точка перед камерой),
//   колесо    — dolly (изменение расстояния до фокуса).
// Двигает узел-владельца; CameraBehaviour на том же узле строит из него view (roll-free).
class OrbitCameraBehaviour : public Behaviour
{
public:
    std::string GetTypeName() const override { return "OrbitCamera"; }

protected:
    void OnStart() override;
    void OnUpdate(float deltaTime) override;

private:
    glm::vec3 ForwardFromAngles() const; // forward по текущим yaw/pitch (как у узла)
    void ApplyToOwner();                 // пишет позицию/поворот узла из состояния

    float _yaw = 0.0f;   // радианы (вокруг Y)
    float _pitch = 0.0f; // радианы (вокруг X)
    float _radius = 3.0f;
    glm::vec3 _target { 0.0f };

    float _orbitSpeed = 0.005f; // рад/пиксель
    float _panSpeed = 0.0025f;  // множитель пана (× radius), мир-ед/пиксель
    float _dollyStep = 0.5f;    // ед. расстояния за «щелчок» колеса

    bool _hasLastMouse = false;
    double _lastX = 0.0;
    double _lastY = 0.0;
    bool _prevRight = false;
};
