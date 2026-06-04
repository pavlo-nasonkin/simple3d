#include "MenuBarPanel.h"

#include <imgui.h>
#include <nfd.hpp>

#include <iostream>
#include <memory>
#include <string>

#include "Scene3D.h"
#include "Pivot3D.h"
#include "models/ExternalModel.h"
#include "models/BoxModel.h"
#include "models/PlaneModel.h"
#include "models/SphereModel.h"
#include "models/CylinderModel.h"
#include "models/ConeModel.h"
#include "models/TorusModel.h"
#include "editor/EditorUI.h"
#include "editor/EditorConfig.h"
#include "scene/Prefab.h"
#include "scene/SceneSerializer.h"

namespace {
    // Создаёт объект типа T, инициализирует и добавляет в сцену.
    template <typename T>
    void AddObject(Scene3D* scene, const char* name) {
        if (!scene) return;
        auto obj = std::make_shared<T>();
        obj->Init();
        obj->SetName(name);
        scene->AddChild(obj);
    }
}

void MenuBarPanel::Draw(Scene3D* scene, Camera* camera, bool* showHierarchy, bool* showInspector, bool* showMaterials)
{
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New")) {
            if (scene) scene->RemoveChildren();
        }
        if (ImGui::MenuItem("Open Scene...")) {
            OpenScene(scene, camera);
        }
        if (ImGui::MenuItem("Save Scene As...")) {
            SaveSceneAs(scene, camera);
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        ImGui::MenuItem("Undo", nullptr, false, false);
        ImGui::MenuItem("Redo", nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Reset Layout")) {
            EditorUI::ResetLayout();
        }
        if (ImGui::BeginMenu("Panels")) {
            ImGui::MenuItem("Hierarchy", nullptr, showHierarchy);
            ImGui::MenuItem("Inspector", nullptr, showInspector);
            ImGui::MenuItem("Materials Inspector", nullptr, showMaterials);
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Import")) {
        if (ImGui::MenuItem("Model...")) {
            ImportModel(scene);
        }
        if (ImGui::MenuItem("Prefab...")) {
            ImportPrefab(scene);
        }
        // Появятся позже:
        ImGui::MenuItem("Material...", nullptr, false, false);
        ImGui::MenuItem("Animation...", nullptr, false, false);
        ImGui::MenuItem("Texture...", nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Create")) {
        if (ImGui::MenuItem("Pivot"))    AddObject<Pivot3D>(scene, "Pivot");
        if (ImGui::MenuItem("Box"))      AddObject<BoxModel>(scene, "Box");
        if (ImGui::MenuItem("Plane"))    AddObject<PlaneModel>(scene, "Plane");
        if (ImGui::MenuItem("Sphere"))   AddObject<SphereModel>(scene, "Sphere");
        if (ImGui::MenuItem("Cylinder")) AddObject<CylinderModel>(scene, "Cylinder");
        if (ImGui::MenuItem("Cone"))     AddObject<ConeModel>(scene, "Cone");
        if (ImGui::MenuItem("Torus"))    AddObject<TorusModel>(scene, "Torus");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        ImGui::MenuItem("About", nullptr, false, false);
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void MenuBarPanel::ImportModel(Scene3D* scene)
{
    if (!scene) {
        return;
    }

    NFD::Guard nfdGuard;       // NFD_Init/NFD_Quit на время вызова
    NFD::UniquePath outPath;
    constexpr nfdfilteritem_t filters[1] = { { "3D Models", "fbx,obj,gltf,glb,dae" } };

    const nfdresult_t result = NFD::OpenDialog(outPath, filters, 1);
    if (result == NFD_OKAY) {
        std::string path = outPath.get();
        // assimp и резолвер текстур ждут прямые слэши (NFD на Windows даёт '\').
        for (char& c : path) {
            if (c == '\\') c = '/';
        }
        auto model = std::make_shared<ExternalModel>(path);
        model->Init();
        scene->AddChild(model);
        std::cout << "[Import] Model: " << path << std::endl;
    } else if (result == NFD_ERROR) {
        std::cout << "[Import] NFD error: " << NFD::GetError() << std::endl;
    }
}

void MenuBarPanel::OpenScene(Scene3D* scene, Camera* camera)
{
    if (!scene || !camera) {
        return;
    }
    NFD::Guard nfdGuard;
    NFD::UniquePath outPath;
    constexpr nfdfilteritem_t filters[1] = { { "Scene", "json" } };
    if (NFD::OpenDialog(outPath, filters, 1) == NFD_OKAY) {
        std::string path = outPath.get();
        for (char& c : path) { if (c == '\\') c = '/'; }
        if (SceneSerializer::Load(*scene, *camera, path)) {
            EditorConfig::SetLastScene(path);
        }
    }
}

void MenuBarPanel::SaveSceneAs(Scene3D* scene, Camera* camera)
{
    if (!scene || !camera) {
        return;
    }
    NFD::Guard nfdGuard;
    NFD::UniquePath outPath;
    constexpr nfdfilteritem_t filters[1] = { { "Scene", "json" } };
    if (NFD::SaveDialog(outPath, filters, 1, nullptr, "scene.json") == NFD_OKAY) {
        std::string path = outPath.get();
        for (char& c : path) { if (c == '\\') c = '/'; }
        if (SceneSerializer::Save(*scene, *camera, path)) {
            EditorConfig::SetLastScene(path);
        }
    }
}

void MenuBarPanel::ImportPrefab(Scene3D* scene)
{
    if (!scene) {
        return;
    }

    NFD::Guard nfdGuard;
    NFD::UniquePath outPath;
    constexpr nfdfilteritem_t filters[1] = { { "Prefab", "json" } };

    const nfdresult_t result = NFD::OpenDialog(outPath, filters, 1);
    if (result == NFD_OKAY) {
        std::string path = outPath.get();
        for (char& c : path) {
            if (c == '\\') c = '/';
        }
        if (auto node = Prefab::Load(path)) {
            scene->AddChild(node);
            std::cout << "[Import] Prefab: " << path << std::endl;
        }
    } else if (result == NFD_ERROR) {
        std::cout << "[Import] NFD error: " << NFD::GetError() << std::endl;
    }
}
