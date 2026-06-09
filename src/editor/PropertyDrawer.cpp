#include "PropertyDrawer.h"

#include <cstring>
#include <string>

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Pivot3D.h"
#include "behaviours/Behaviour.h"
#include "behaviours/reflection/Property.h"
#include "behaviours/reflection/References.h"

namespace {

// Принимает payload "NODE_ID" из Hierarchy на последний нарисованный виджет.
// Возвращает id перетащенного узла или 0.
unsigned int AcceptNodeDrop()
{
    unsigned int dropped = 0;
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_ID")) {
            dropped = *static_cast<const unsigned int*>(payload->Data);
        }
        ImGui::EndDragDropTarget();
    }
    return dropped;
}

void DrawNodeRefField(const char* label, NodeRef& ref, Pivot3D* root)
{
    // Ленивый резолв кэша по id (после загрузки сцены / drag-n-drop).
    if (ref.id != 0 && ref.cached.expired() && root) {
        ref.cached = root->GetChildById(ref.id);
    }

    const auto target = ref.Get();
    std::string text = target ? target->GetName()
                     : (ref.id ? ("(id " + std::to_string(ref.id) + ")") : std::string("(None)"));

    ImGui::Button(text.c_str());
    if (const unsigned int id = AcceptNodeDrop()) {
        ref.id = id;
        ref.cached = root ? root->GetChildById(id) : std::shared_ptr<Pivot3D>();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    if (ref.IsSet()) {
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) {
            ref.Clear();
        }
    }
}

void DrawBehRefField(const char* label, BehRef& ref, Pivot3D* root)
{
    if (ref.nodeId != 0 && ref.cached == nullptr && root) {
        if (auto node = root->GetChildById(ref.nodeId)) {
            ref.cached = ref.type.empty()
                ? (node->GetBehaviours().empty() ? nullptr : node->GetBehaviours().front().get())
                : node->FindBehaviourByType(ref.type);
        }
    }

    std::string text = ref.cached
        ? (ref.cached->GetTypeName() + " @node " + std::to_string(ref.nodeId))
        : (ref.nodeId ? ("(node " + std::to_string(ref.nodeId) + ")") : std::string("(None)"));

    ImGui::Button(text.c_str());
    if (const unsigned int id = AcceptNodeDrop()) {
        if (auto node = root ? root->GetChildById(id) : nullptr) {
            Behaviour* first = node->GetBehaviours().empty() ? nullptr : node->GetBehaviours().front().get();
            ref.nodeId = id;
            ref.cached = first;
            ref.type = first ? first->GetTypeName() : std::string();
        }
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    if (ref.IsSet()) {
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) {
            ref.Clear();
        }
    }
}

void DrawScalar(const char* label, PropertyType type, void* field, Pivot3D* root)
{
    switch (type) {
    case PropertyType::Bool:
        ImGui::Checkbox(label, static_cast<bool*>(field));
        break;
    case PropertyType::Int:
        ImGui::DragInt(label, static_cast<int*>(field));
        break;
    case PropertyType::Float:
        ImGui::DragFloat(label, static_cast<float*>(field), 0.05f);
        break;
    case PropertyType::String: {
        auto* str = static_cast<std::string*>(field);
        char buffer[256];
        std::strncpy(buffer, str->c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText(label, buffer, sizeof(buffer))) {
            *str = buffer;
        }
        break;
    }
    case PropertyType::Vec3:
        ImGui::DragFloat3(label, glm::value_ptr(*static_cast<glm::vec3*>(field)), 0.05f);
        break;
    case PropertyType::Color:
        ImGui::ColorEdit3(label, glm::value_ptr(*static_cast<glm::vec3*>(field)));
        break;
    case PropertyType::NodeRef:
        DrawNodeRefField(label, *static_cast<NodeRef*>(field), root);
        break;
    case PropertyType::BehRef:
        DrawBehRefField(label, *static_cast<BehRef*>(field), root);
        break;
    case PropertyType::List:
        break; // обрабатывается отдельно в DrawBehaviourProperties
    }
}

} // namespace

void editor::DrawBehaviourProperties(Behaviour* behaviour, Pivot3D* contextNode)
{
    if (!behaviour) {
        return;
    }
    Pivot3D* root = contextNode ? contextNode->Root() : nullptr;

    for (const Property& prop : behaviour->GetProperties()) {
        ImGui::PushID(prop.name.c_str());
        void* field = prop.resolve(behaviour);

        if (prop.type == PropertyType::List) {
            if (ImGui::TreeNodeEx(prop.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                const std::size_t count = prop.listSize(field);
                std::ptrdiff_t removeIndex = -1;
                for (std::size_t i = 0; i < count; ++i) {
                    ImGui::PushID(static_cast<int>(i));
                    void* element = prop.listAt(field, i);
                    const std::string elementLabel = "[" + std::to_string(i) + "]";
                    DrawScalar(elementLabel.c_str(), prop.elementType, element, root);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("-")) {
                        removeIndex = static_cast<std::ptrdiff_t>(i);
                    }
                    ImGui::PopID();
                }
                if (removeIndex >= 0) {
                    prop.listRemove(field, static_cast<std::size_t>(removeIndex));
                }
                if (ImGui::SmallButton("+ Add")) {
                    prop.listAdd(field);
                }
                ImGui::TreePop();
            }
        } else {
            DrawScalar(prop.name.c_str(), prop.type, field, root);
        }

        ImGui::PopID();
    }
}
