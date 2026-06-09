#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

// Лёгкая рефлексия свойств Behaviour (вариант A: ручная регистрация без кодогена).
// Каждое свойство описывается типобезопасно через указатель-на-член; это единый
// источник истины и для инспектора, и (в будущем) для сериализации (9.8/9.9).
//
// Использование (внутри метода GetProperties() компонента):
//   static const std::vector<Property> props = {
//       REFLECT_PROPERTY(MyBehaviour, _numBullets, PropertyType::Int),
//       REFLECT_LIST(MyBehaviour, _points, PropertyType::NodeRef),
//   };

enum class PropertyType
{
    Bool,
    Int,
    Float,
    String,
    Vec3,
    Color,
    NodeRef,
    BehRef,
    List
};

struct Property
{
    std::string name;
    PropertyType type = PropertyType::Int;
    PropertyType elementType = PropertyType::Int; // действителен при type == List

    // instance -> указатель на поле (скаляр) либо на std::vector<T> (List).
    std::function<void*(void*)> resolve;

    // Операции для List (заполняются только при type == List). Аргумент — указатель
    // на сам std::vector<T> (результат resolve), элемент — указатель на T.
    std::function<std::size_t(void*)> listSize;
    std::function<void*(void*, std::size_t)> listAt;
    std::function<void(void*)> listAdd;
    std::function<void(void*, std::size_t)> listRemove;
    std::function<void(void*)> listClear;
};

namespace reflection {

template <class C, class M>
Property MakeProperty(std::string name, PropertyType type, M C::* member)
{
    Property p;
    p.name = std::move(name);
    p.type = type;
    p.resolve = [member](void* instance) -> void* {
        return static_cast<void*>(&(static_cast<C*>(instance)->*member));
    };
    return p;
}

template <class C, class T>
Property MakeListProperty(std::string name, PropertyType elementType, std::vector<T> C::* member)
{
    Property p;
    p.name = std::move(name);
    p.type = PropertyType::List;
    p.elementType = elementType;
    p.resolve = [member](void* instance) -> void* {
        return static_cast<void*>(&(static_cast<C*>(instance)->*member));
    };
    p.listSize = [](void* vec) -> std::size_t {
        return static_cast<std::vector<T>*>(vec)->size();
    };
    p.listAt = [](void* vec, std::size_t i) -> void* {
        return static_cast<void*>(&(*static_cast<std::vector<T>*>(vec))[i]);
    };
    p.listAdd = [](void* vec) {
        static_cast<std::vector<T>*>(vec)->emplace_back();
    };
    p.listRemove = [](void* vec, std::size_t i) {
        auto* v = static_cast<std::vector<T>*>(vec);
        v->erase(v->begin() + static_cast<std::ptrdiff_t>(i));
    };
    p.listClear = [](void* vec) { static_cast<std::vector<T>*>(vec)->clear(); };
    return p;
}

} // namespace reflection

#define REFLECT_PROPERTY(Class, field, propType) \
    reflection::MakeProperty(#field, propType, &Class::field)

#define REFLECT_LIST(Class, field, elemType) \
    reflection::MakeListProperty(#field, elemType, &Class::field)
