#include "Prefab.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>

#include "Pivot3D.h"
#include "render/Geometry.h"
#include "materials/Material3D.h"
#include "scene/Serialization.h"

using json = nlohmann::json;

namespace {
    std::string BinPathFor(const std::string& jsonPath) {
        return std::filesystem::path(jsonPath).replace_extension(".bin").string();
    }
}

bool Prefab::Save(const Pivot3D& root, const std::string& jsonPath)
{
    SceneIO::SaveContext ctx;
    json doc;
    doc["version"] = 1;
    doc["root"] = SceneIO::NodeToJson(ctx, root);
    doc["geometries"] = ctx.geometries;
    doc["materials"] = ctx.materials;

    const std::string binPath = BinPathFor(jsonPath);
    doc["binary"] = std::filesystem::path(binPath).filename().string();

    std::ofstream binFile(binPath, std::ios::binary);
    if (!binFile) {
        std::cout << "[Prefab] cannot write " << binPath << std::endl;
        return false;
    }
    binFile.write(reinterpret_cast<const char*>(ctx.bin.data()), static_cast<std::streamsize>(ctx.bin.size()));
    binFile.close();

    std::ofstream jsonFile(jsonPath);
    if (!jsonFile) {
        std::cout << "[Prefab] cannot write " << jsonPath << std::endl;
        return false;
    }
    jsonFile << doc.dump(2);
    return true;
}

std::shared_ptr<Pivot3D> Prefab::Load(const std::string& jsonPath)
{
    std::ifstream jsonFile(jsonPath);
    if (!jsonFile) {
        std::cout << "[Prefab] cannot open " << jsonPath << std::endl;
        return nullptr;
    }

    json doc;
    try {
        jsonFile >> doc;
    } catch (const std::exception& e) {
        std::cout << "[Prefab] json parse error: " << e.what() << std::endl;
        return nullptr;
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

    if (!doc.contains("root")) {
        return nullptr;
    }
    return SceneIO::NodeFromJson(doc.at("root"), geometries, materials);
}
