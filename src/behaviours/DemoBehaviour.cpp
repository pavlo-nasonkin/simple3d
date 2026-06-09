#include "DemoBehaviour.h"

const std::vector<Property>& DemoBehaviour::GetProperties() const
{
    static const std::vector<Property> props = {
        REFLECT_PROPERTY(DemoBehaviour, _enabledFlag, PropertyType::Bool),
        REFLECT_PROPERTY(DemoBehaviour, _numBullets,  PropertyType::Int),
        REFLECT_PROPERTY(DemoBehaviour, _fireRate,    PropertyType::Float),
        REFLECT_PROPERTY(DemoBehaviour, _label,       PropertyType::String),
        REFLECT_PROPERTY(DemoBehaviour, _tint,        PropertyType::Color),
        REFLECT_PROPERTY(DemoBehaviour, _target,      PropertyType::NodeRef),
        REFLECT_PROPERTY(DemoBehaviour, _controller,  PropertyType::BehRef),
        REFLECT_LIST(DemoBehaviour, _ammoPerClip,  PropertyType::Int),
        REFLECT_LIST(DemoBehaviour, _cooldowns,    PropertyType::Float),
        REFLECT_LIST(DemoBehaviour, _patrolPoints, PropertyType::NodeRef),
    };
    return props;
}
