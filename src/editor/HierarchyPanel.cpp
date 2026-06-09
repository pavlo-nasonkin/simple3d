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

        // Выбор — по релизу клика без перетаскивания (а не по нажатию): иначе старт
        // drag мгновенно переключал бы выбранный узел и инспектор уходил бы с поля,
        // в которое тащим. GetMouseDragDelta возвращает (0,0), пока курсор не ушёл за
        // порог перетаскивания — так отличаем клик от drag (публичный API ImGui).
        if (ImGui::IsItemHovered()
            && ImGui::IsMouseReleased(ImGuiMouseButton_Left)
            && !ImGui::IsItemToggledOpen()) {
            const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            if (drag.x * drag.x + drag.y * drag.y < 25.0f) { // < ~5px → это клик, не drag
                selector->SetSelectedObject(node);
            }
        }

        // Источник drag-n-drop: тащим id узла (для полей NodeRef/BehRef в инспекторе).
        if (ImGui::BeginDragDropSource()) {
            const unsigned int id = node->GetId();
            ImGui::SetDragDropPayload("NODE_ID", &id, sizeof(id));
            ImGui::TextUnformatted(name.c_str());
            ImGui::EndDragDropSource();
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
