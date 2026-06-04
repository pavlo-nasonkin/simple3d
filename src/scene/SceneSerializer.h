#pragma once

#include <string>

class Scene3D;
class Camera;

// Сохранение/загрузка сцены: узлы (с запечённой геометрией и материалами через SceneIO),
// окружение (directional light, ambient, тени, HDR) и камера.
// Формат: <base>.json + <base>.bin.
class SceneSerializer
{
public:
    static bool Save(Scene3D& scene, const Camera& camera, const std::string& jsonPath);
    static bool Load(Scene3D& scene, Camera& camera, const std::string& jsonPath);
};
