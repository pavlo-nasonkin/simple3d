#include "Behaviour.h"

const std::vector<Property>& Behaviour::GetProperties() const
{
    static const std::vector<Property> kEmpty;
    return kEmpty;
}

void Behaviour::SetEnabled(bool enabled)
{
    if (_enabled == enabled) {
        return;
    }
    _enabled = enabled;
    if (_enabled) {
        OnEnable();
    } else {
        OnDisable();
    }
}

void Behaviour::Attach(Pivot3D* owner)
{
    _owner = owner;
    OnAttach();
}

void Behaviour::Detach()
{
    OnDetach();
    _owner = nullptr;
}

void Behaviour::Tick(float deltaTime)
{
    if (!_enabled) {
        return;
    }
    if (!_started) {
        _started = true;
        OnStart();
    }
    OnUpdate(deltaTime);
}
