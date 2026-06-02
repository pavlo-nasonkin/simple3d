#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "render/Geometry.h"

// Кэш геометрии по строковому ключу через weak_ptr: пока жив хотя бы один Mesh,
// использующий Geometry, она переиспользуется; когда исчезает последний — Geometry
// (и её VBO/VAO в VRAM) удаляется автоматически, без явного вызова.
class GeometryRegistry
{
    std::map<std::string, std::weak_ptr<Geometry>, std::less<>> _cache;

public:
    template <typename Factory>
    std::shared_ptr<Geometry> GetOrCreate(std::string_view key, Factory&& factory)
    {
        if (auto it = _cache.find(key); it != _cache.end()) {
            if (auto alive = it->second.lock()) {
                return alive;
            }
        }
        auto geometry = std::make_shared<Geometry>(std::forward<Factory>(factory)());
        _cache[std::string(key)] = geometry; // хранится как weak_ptr
        return geometry;
    }

    void Cleanup() { _cache.clear(); }
};
