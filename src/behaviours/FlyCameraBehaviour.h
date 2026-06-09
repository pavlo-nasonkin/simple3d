#pragma once

#include <string>

#include "Behaviour.h"

// Контроллер свободной камеры: при зажатой ПКМ — обзор мышью + перемещение WASD
// (Q/E — вниз/вверх). Двигает и поворачивает узел-владельца; CameraBehaviour на том
// же узле превращает его трансформ во view. Навигация только под ПКМ — чтобы клики/
// ввод в панелях редактора не дёргали камеру.
//
// Опрос ввода идёт из Engine (поллинг), без подписки на listener'ы — как задумано
// для Behaviour'ов (DEVELOP_PLAN 9.4).
class FlyCameraBehaviour : public Behaviour
{
public:
    void SetMoveSpeed(float unitsPerSec) { _moveSpeed = unitsPerSec; }
    void SetLookSensitivity(float radPerPixel) { _lookSensitivity = radPerPixel; }

    std::string GetTypeName() const override { return "FlyCamera"; }

protected:
    void OnStart() override;
    void OnUpdate(float deltaTime) override;

private:
    float _yaw = 0.0f;   // радианы (вокруг Y)
    float _pitch = 0.0f; // радианы (вокруг X)
    float _moveSpeed = 5.0f;
    float _lookSensitivity = 0.0025f;
    bool _hasLastMouse = false;
    double _lastX = 0.0;
    double _lastY = 0.0;
};
