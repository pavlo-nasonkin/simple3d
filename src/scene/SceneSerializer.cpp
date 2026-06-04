#include "SceneSerializer.h"

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>

#include "Scene3D.h"
#include "Pivot3D.h"
#include "camera/Camera.h"
#include "scene/Serialization.h"

using json = nlohmann::json;

namespace {
    json Vec3ToJson(const glm::vec3& v) { return json::array({ v.x, v.y, v.z }); }
    glm::vec3 Vec3FromJson(const json& j, const glm::vec3& fallback = glm::vec3(0.0f)) {
        if (!j.is_array() || j.size() < 3) return fallback;
        return glm::vec3(j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>());
    }

    std::string BinPathFor(const std::string& jsonPath) {
        return std::filesystem::path(jsonPath).replace_extension(".bin").string();
    }
}

bool SceneSerializer::Save(Scene3D& scene, const Camera& camera, const std::string& jsonPath)
{
    SceneIO::SaveContext ctx;
    json doc;
    doc["version"] = 1;

    json nodes = json::array();
    for (const auto& child : scene.Children()) {
        nodes.push_back(SceneIO::NodeToJson(ctx, *child));
    }
    doc["nodes"] = nodes;
    doc["geometries"] = ctx.geometries;
    doc["materials"] = ctx.materials;

    doc["environment"] = {
        { "dirLight", {
            { "direction", Vec3ToJson(*scene.GetDirLightDirection()) },
            { "color",     Vec3ToJson(*scene.GetDirLightColor()) },
            { "intensity", scene.GetDirLightIntensity() },
        }},
        { "ambient", Vec3ToJson(*scene.GetLightAmbient()) },
        { "shadow", {
            { "radius",   scene.GetShadowRadius() },
            { "strength", scene.GetShadowStrength() },
            { "center",   Vec3ToJson(scene.GetShadowCenter()) },
        }},
        { "hdr", scene.GetHdrPath() },
    };

    doc["camera"] = {
        { "position", Vec3ToJson(camera.Position) },
        { "yaw", camera.Yaw },
        { "pitch", camera.Pitch },
    };

    const std::string binPath = BinPathFor(jsonPath);
    doc["binary"] = std::filesystem::path(binPath).filename().string();

    std::ofstream binFile(binPath, std::ios::binary);
    if (!binFile) {
        std::cout << "[Scene] cannot write " << binPath << std::endl;
        return false;
    }
    binFile.write(reinterpret_cast<const char*>(ctx.bin.data()), static_cast<std::streamsize>(ctx.bin.size()));
    binFile.close();

    std::ofstream jsonFile(jsonPath);
    if (!jsonFile) {
        std::cout << "[Scene] cannot write " << jsonPath << std::endl;
        return false;
    }
    jsonFile << doc.dump(2);
    std::cout << "[Scene] saved: " << jsonPath << std::endl;
    return true;
}

bool SceneSerializer::Load(Scene3D& scene, Camera& camera, const std::string& jsonPath)
{
    std::ifstream jsonFile(jsonPath);
    if (!jsonFile) {
        std::cout << "[Scene] cannot open " << jsonPath << std::endl;
        return false;
    }

    json doc;
    try {
        jsonFile >> doc;
    } catch (const std::exception& e) {
        std::cout << "[Scene] json parse error: " << e.what() << std::endl;
        return false;
    }

    const std::string binPath = BinPathFor(jsonPath);
    std::ifstream binFile(binPath, std::ios::binary | std::ios::ate);
    std::vector<std::byte> bin;
    if (binFile) {
        const std::streamsize size = binFile.tellg();
        binFile.seekg(0);
        bin.resize(static_cast<size_t>(size));
        binFile.read(reinterpret_cast<char*>(bin.data()), size);
    }

    auto geometries = SceneIO::BuildGeometries(doc.value("geometries", json::array()), bin);
    auto materials = SceneIO::BuildMaterials(doc.value("materials", json::array()));

    // Заменяем содержимое сцены.
    scene.RemoveChildren();
    for (const json& nj : doc.value("nodes", json::array())) {
        scene.AddChild(SceneIO::NodeFromJson(nj, geometries, materials));
    }

    if (doc.contains("environment")) {
        const json& env = doc.at("environment");
        if (env.contains("dirLight")) {
            const json& d = env.at("dirLight");
            scene.SetDirLightDirection(Vec3FromJson(d.value("direction", json()), *scene.GetDirLightDirection()));
            scene.SetDirLightColor(Vec3FromJson(d.value("color", json()), *scene.GetDirLightColor()));
            scene.SetDirLightIntensity(d.value("intensity", scene.GetDirLightIntensity()));
        }
        scene.SetLightAmbient(Vec3FromJson(env.value("ambient", json()), *scene.GetLightAmbient()));
        if (env.contains("shadow")) {
            const json& s = env.at("shadow");
            scene.SetShadowArea(Vec3FromJson(s.value("center", json()), scene.GetShadowCenter()),
                                s.value("radius", scene.GetShadowRadius()));
            scene.SetShadowStrength(s.value("strength", scene.GetShadowStrength()));
        }
        const std::string hdr = env.value("hdr", std::string());
        if (!hdr.empty()) {
            scene.SetEnvironmentFromHdr(hdr);
        }
    }

    if (doc.contains("camera")) {
        const json& c = doc.at("camera");
        camera.Position = Vec3FromJson(c.value("position", json()), camera.Position);
        camera.SetOrientation(c.value("yaw", camera.Yaw), c.value("pitch", camera.Pitch));
    }

    std::cout << "[Scene] loaded: " << jsonPath << std::endl;
    return true;
}
