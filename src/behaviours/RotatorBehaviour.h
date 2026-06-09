#pragma once

#include <string>
#include <glm/glm.hpp>

#include "Behaviour.h"

// Простейший компонент: вращает узел-владельца вокруг заданной оси с заданной
// скоростью. Кадронезависим (умножает на deltaTime). Служит смоук-тестом каркаса
// Behaviour: проверяет OnUpdate + запись в трансформ узла.
class RotatorBehaviour : public Behaviour
{
public:
    RotatorBehaviour() = default;
    RotatorBehaviour(const glm::vec3& axis, float speedRadPerSec)
        : _axis(axis), _speed(speedRadPerSec) {}

    void SetAxis(const glm::vec3& axis) { _axis = axis; }
    const glm::vec3& GetAxis() const { return _axis; }

    void SetSpeed(float radPerSec) { _speed = radPerSec; }
    float GetSpeed() const { return _speed; }

    std::string GetTypeName() const override { return "Rotator"; }
    const std::vector<Property>& GetProperties() const override;

protected:
    void OnUpdate(float deltaTime) override;

private:
    glm::vec3 _axis { 0.0f, 1.0f, 0.0f }; // ось вращения (нормализуется неявно через компоненты)
    float _speed = 1.0f;                  // радиан/сек
};
