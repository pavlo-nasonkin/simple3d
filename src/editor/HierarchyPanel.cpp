#include "HierarchyPanel.h"

#include <imgui.h>
#include <memory>

#include "Engine.h"
#include "Scene3D.h"
#include "Pivot3D.h"
#include "object_selector/ObjectSelector.h"

namespace {
    void DrawNode(const std::shared_ptr<Pivot3D>& node, ObjectSelector* selector)
    {
        const auto& children = node->Children();
        const bool isLeaf = children.empty();
        const bool isSelected = (selector->GetSelectedObject() == node);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (isLeaf)     flags |= ImGuiTreeNodeFlags_Leaf;
        if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

        const std::string& name = node->GetName();
        const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(node->GetId())),
                                            flags, "%s", name.c_str());

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            selector->SetSelectedObject(node);
        }

        if (open) {
            for (const auto& child : children) {
                DrawNode(child, selector);
            }
            ImGui::TreePop();
        }
    }
}

void HierarchyPanel::Draw(Scene3D* scene, bool* open)
{
    if (open && !*open) {
        return;
    }
    if (!ImGui::Begin("Hierarchy", open)) {
        ImGui::End();
        return;
    }

    if (scene) {
        ObjectSelector* selector = Engine::GetInstance().GetObjectSelector().get();
        if (selector) {
            for (const auto& child : scene->Children()) {
                DrawNode(child, selector);
            }
        }
    }

    ImGui::End();
}
