#include "InspectorPanel.h"

#include <imgui.h>
#include <nfd.hpp>
#include <glm/glm.hpp>
#include <cstring>
#include <iostream>
#include <memory>

#include "Engine.h"
#include "Pivot3D.h"
#include "object_selector/ObjectSelector.h"
#include "scene/Prefab.h"

void InspectorPanel::Draw(bool* open)
{
    if (open && !*open) {
        return;
    }
    if (!ImGui::Begin("Inspector", open)) {
        ImGui::End();
        return;
    }

    ObjectSelector* selector = Engine::GetInstance().GetObjectSelector().get();
    std::shared_ptr<Pivot3D> node = selector ? selector->GetSelectedObject() : nullptr;

    if (!node) {
        ImGui::TextDisabled("Ничего не выбрано");
        ImGui::End();
        return;
    }

    // Имя
    char nameBuf[128];
    std::strncpy(nameBuf, node->GetName().c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
        node->SetName(nameBuf);
    }

    ImGui::SeparatorText("Transform");

    glm::vec3 position = *node->GetPosition();
    if (ImGui::DragFloat3("Position", &position.x, 0.05f)) {
        node->SetPosition(position.x, position.y, position.z);
    }

    glm::vec3 rotationDeg = glm::degrees(*node->GetRotation());
    if (ImGui::DragFloat3("Rotation", &rotationDeg.x, 0.5f)) {
        const glm::vec3 r = glm::radians(rotationDeg);
        node->SetRotation(r.x, r.y, r.z);
    }

    glm::vec3 scale = *node->GetScale();
    if (ImGui::DragFloat3("Scale", &scale.x, 0.01f)) {
        node->SetScale(scale.x, scale.y, scale.z);
    }

    ImGui::SeparatorText("Shadows");

    bool castShadows = node->GetCastShadows();
    if (ImGui::Checkbox("Cast shadows", &castShadows)) {
        node->SetCastShadows(castShadows);
    }

    bool receiveShadows = node->GetReceiveShadows();
    if (ImGui::Checkbox("Receive shadows", &receiveShadows)) {
        node->SetReceiveShadows(receiveShadows);
    }

    ImGui::SeparatorText("Prefab");
    if (ImGui::Button("Save as Prefab...")) {
        NFD::Guard nfdGuard;
        NFD::UniquePath outPath;
        constexpr nfdfilteritem_t filters[1] = { { "Prefab", "json" } };
        if (NFD::SaveDialog(outPath, filters, 1, nullptr, "prefab.json") == NFD_OKAY) {
            std::string path = outPath.get();
            for (char& c : path) {
                if (c == '\\') c = '/';
            }
            if (Prefab::Save(*node, path)) {
                std::cout << "[Prefab] saved: " << path << std::endl;
            }
        }
    }

    ImGui::End();
}
