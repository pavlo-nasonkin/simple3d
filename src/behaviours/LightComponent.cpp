#include "LightComponent.h"

#include "Pivot3D.h"

DirectionalLightComponent::DirectionalLightComponent()
{
    _color = glm::vec3(1.0f, 0.96f, 0.90f); // тёплый тон солнца
    _intensity = 3.0f;                       // компенсация деления на π в PBR
}

const std::vector<Property>& DirectionalLightComponent::GetProperties() const
{
    static const std::vector<Property> props = {
        REFLECT_PROPERTY(DirectionalLightComponent, _color,     PropertyType::Color),
        REFLECT_PROPERTY(DirectionalLightComponent, _intensity, PropertyType::Float),
        REFLECT_PROPERTY(DirectionalLightComponent, _ambient,   PropertyType::Color),
    };
    return props;
}

glm::vec3 DirectionalLightComponent::GetWorldDirection() const
{
    if (Pivot3D* owner = GetOwner()) {
        const glm::mat4 world = owner->WorldMatrix();
        return glm::normalize(-glm::vec3(world[2])); // forward = −Z
    }
    return glm::vec3(0.0f, -1.0f, 0.0f);
}

const std::vector<Property>& PointLightComponent::GetProperties() const
{
    static const std::vector<Property> props = {
        REFLECT_PROPERTY(PointLightComponent, _color,     PropertyType::Color),
        REFLECT_PROPERTY(PointLightComponent, _intensity, PropertyType::Float),
        REFLECT_PROPERTY(PointLightComponent, _range,     PropertyType::Float),
    };
    return props;
}

glm::vec3 PointLightComponent::GetWorldPosition() const
{
    if (Pivot3D* owner = GetOwner()) {
        return glm::vec3(owner->WorldMatrix()[3]);
    }
    return glm::vec3(0.0f);
}
