#pragma once

#include <memory>
#include <string>

class Pivot3D;
class Behaviour;

// Ссылка на узел сцены. Сериализуется по стабильному id; в рантайме держит weak_ptr
// (резолвится после загрузки сцены или при drag-n-drop из Hierarchy).
struct NodeRef
{
    unsigned int id = 0;
    std::weak_ptr<Pivot3D> cached;

    std::shared_ptr<Pivot3D> Get() const { return cached.lock(); }
    bool IsSet() const { return id != 0; }
    void Clear() { id = 0; cached.reset(); }
};

// Ссылка на компонент (Behaviour) на узле. Сериализуется как (nodeId + тип компонента);
// в рантайме — сырой указатель (компонент живёт не дольше узла-владельца).
struct BehRef
{
    unsigned int nodeId = 0;
    std::string type;            // GetTypeName() целевого компонента
    Behaviour* cached = nullptr; // не владеет

    Behaviour* Get() const { return cached; }
    bool IsSet() const { return nodeId != 0; }
    void Clear() { nodeId = 0; type.clear(); cached = nullptr; }
};
