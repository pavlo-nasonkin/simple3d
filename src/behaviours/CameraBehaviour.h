#pragma once

#include <memory>
#include <string>

#include "Behaviour.h"

class Camera;

// Камера как компонент узла (сенсор). View выводится из мировой матрицы владельца
// (view = inverse(worldMatrix)), поэтому камера живёт в графе: имеет трансформ,
// родителя (следует за ним), сериализуется и видна в редакторе.
//
// Для совместимости с остальным пайплайном (RenderContext.camera, ViewportPanel,
// ObjectSelector — все на Camera*) держит общий объект Camera и синхронизирует его
// с узлом каждый кадр (Refresh). Управление вынесено в отдельный контроллер
// (FlyCameraBehaviour), который двигает узел.
//
// Известное ограничение v1: ориентация берётся из Euler-узла (порядок Rx*Ry*Rz);
// при экстремальных комбинациях pitch+yaw возможен лёгкий roll. Проекция пока
// строится вызывающим кодом/SetViewportSize (см. также REFACTORING_PLAN 7.1).
class CameraBehaviour : public Behaviour
{
public:
    CameraBehaviour();
    CameraBehaviour(float fovYDeg, float nearPlane, float farPlane);

    const std::shared_ptr<Camera>& GetCamera() const { return _camera; }

    void SetPerspective(float fovYDeg, float nearPlane, float farPlane);
    float GetFovYDeg() const { return _fovYDeg; }
    float GetNearPlane() const { return _nearPlane; }
    float GetFarPlane() const { return _farPlane; }

    // Пересобирает проекцию из fov/near/far под размер цели (вьюпорт/окно).
    void SetViewportSize(float width, float height);

    // Синхронизирует внутренний Camera с мировой матрицей узла (view = inverse(world)).
    void Refresh();

    std::string GetTypeName() const override { return "Camera"; }

protected:
    void OnAttach() override;
    void OnUpdate(float deltaTime) override;

private:
    std::shared_ptr<Camera> _camera;
    float _fovYDeg = 45.0f;
    float _nearPlane = 0.1f;
    float _farPlane = 100.0f;
};
