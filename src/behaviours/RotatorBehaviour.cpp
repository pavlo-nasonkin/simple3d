#include "RotatorBehaviour.h"

#include "Pivot3D.h"

const std::vector<Property>& RotatorBehaviour::GetProperties() const
{
    static const std::vector<Property> props = {
        REFLECT_PROPERTY(RotatorBehaviour, _axis,  PropertyType::Vec3),
        REFLECT_PROPERTY(RotatorBehaviour, _speed, PropertyType::Float),
    };
    return props;
}

void RotatorBehaviour::OnUpdate(float deltaTime)
{
    Pivot3D* owner = GetOwner();
    if (!owner) {
        return;
    }
    const float step = _speed * deltaTime;
    owner->Rotate(_axis.x * step, _axis.y * step, _axis.z * step);
}
