#pragma once

#include <string>
#include <vector>

#include "reflection/Property.h"

class Pivot3D;

// Компонент логики, прикрепляемый к узлу сцены (Pivot3D) — аналог Unity MonoBehaviour.
// Жизненный цикл вызывает движок (через Pivot3D / Scene3D::Update), а не пользователь:
//   OnAttach  — владелец установлен, узел уже в графе;
//   OnStart   — один раз, перед первым OnUpdate;
//   OnUpdate  — каждый кадр, пока enabled и узел active;
//   OnDetach  — перед удалением компонента / уничтожением узла;
//   OnEnable/OnDisable — на переключении IsEnabled().
//
// Здесь НЕТ рендера и GL — это чистая логика. Рендер остаётся в Mesh/Material.
class Behaviour
{
    friend class Pivot3D;

public:
    virtual ~Behaviour() = default;

    Pivot3D* GetOwner() const { return _owner; }

    bool IsEnabled() const { return _enabled; }
    void SetEnabled(bool enabled);

    // Строковый тип для сериализации/инспектора (как Filter3D::GetTypeName).
    virtual std::string GetTypeName() const = 0;

    // Описание редактируемых полей (для инспектора/сериализации). По умолчанию пусто.
    virtual const std::vector<Property>& GetProperties() const;

protected:
    virtual void OnAttach() {}
    virtual void OnStart() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnDetach() {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}

private:
    // Вызываются только из Pivot3D (friend):
    void Attach(Pivot3D* owner); // ставит владельца + OnAttach
    void Detach();               // OnDetach + сброс владельца
    void Tick(float deltaTime);  // OnStart (один раз) + OnUpdate, если enabled

    Pivot3D* _owner = nullptr;
    bool _enabled = true;
    bool _started = false;
};
