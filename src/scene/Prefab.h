#pragma once

#include <memory>
#include <string>

class Pivot3D;

// Сохранение/загрузка самодостаточного префаба:
//   <base>.json — метаданные (узлы, материалы, описания геометрии),
//   <base>.bin  — запечённые бинарные VBO/EBO (и bone-буферы).
// Префаб не зависит от исходной модели и assimp при загрузке.
class Prefab
{
public:
    // jsonPath — путь к .json (бинарь берётся как тот же путь с расширением .bin).
    static bool Save(const Pivot3D& root, const std::string& jsonPath);
    static std::shared_ptr<Pivot3D> Load(const std::string& jsonPath);
};
