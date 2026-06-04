#pragma once

#include <nlohmann/json.hpp>

#include <cstddef>
#include <map>
#include <memory>
#include <vector>

class Pivot3D;
class Geometry;
class Material3D;

// Общая сериализация узлов сцены/префаба: дерево узлов + запечённая геометрия + материалы.
// Используется и Prefab, и SceneSerializer.
namespace SceneIO {

struct SaveContext {
    std::vector<std::byte> bin;                 // бинарные блобы геометрии
    nlohmann::json geometries = nlohmann::json::array();
    nlohmann::json materials  = nlohmann::json::array();
    std::map<const Geometry*, int> geometryIds;
    std::map<const Material3D*, int> materialIds;
};

// Сериализует узел (рекурсивно), регистрируя его геометрию/материалы в ctx.
nlohmann::json NodeToJson(SaveContext& ctx, const Pivot3D& node);

// Восстановление ассетов из json + бинарь.
std::vector<std::shared_ptr<Geometry>> BuildGeometries(const nlohmann::json& geometries,
                                                       const std::vector<std::byte>& bin);
std::vector<std::shared_ptr<Material3D>> BuildMaterials(const nlohmann::json& materials);

// Восстановление узла (рекурсивно) по ранее построенным ассетам.
std::shared_ptr<Pivot3D> NodeFromJson(const nlohmann::json& node,
                                      const std::vector<std::shared_ptr<Geometry>>& geometries,
                                      const std::vector<std::shared_ptr<Material3D>>& materials);

} // namespace SceneIO
