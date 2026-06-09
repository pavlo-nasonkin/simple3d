#include "BehaviourFactory.h"

#include "Behaviour.h"
#include "RotatorBehaviour.h"
#include "DemoBehaviour.h"
#include "LightComponent.h"

std::map<std::string, BehaviourFactory::Creator>& BehaviourFactory::registry()
{
    static std::map<std::string, Creator> instance;
    return instance;
}

void BehaviourFactory::Register(const std::string& type, Creator creator)
{
    registry()[type] = std::move(creator);
}

std::unique_ptr<Behaviour> BehaviourFactory::Create(const std::string& type)
{
    auto it = registry().find(type);
    if (it == registry().end()) {
        return nullptr;
    }
    return it->second();
}

bool BehaviourFactory::IsRegistered(const std::string& type)
{
    return registry().find(type) != registry().end();
}

const std::map<std::string, BehaviourFactory::Creator>& BehaviourFactory::Registry()
{
    return registry();
}

void BehaviourFactory::RegisterBuiltins()
{
    Register("Rotator", [] { return std::make_unique<RotatorBehaviour>(); });
    Register("Demo",    [] { return std::make_unique<DemoBehaviour>(); });
    Register("DirectionalLight", [] { return std::make_unique<DirectionalLightComponent>(); });
    Register("PointLight",       [] { return std::make_unique<PointLightComponent>(); });
}
