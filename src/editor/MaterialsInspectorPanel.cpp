#include "MaterialsInspectorPanel.h"

#include <imgui.h>
#include <memory>
#include <string>

#include "Engine.h"
#include "Pivot3D.h"
#include "models/Mesh.h"
#include "object_selector/ObjectSelector.h"
#include "materials/Material3D.h"
#include "materials/filters/Filter3d.h"
#include "materials/filters/ColorFilter.h"
#include "materials/filters/TextureFilterBase.h"
#include "resources/Texture2D.h"

namespace {
    const char* kSlotNames[] = {
        "BaseColor", "Specular", "Normal", "Emissive", "Overlay", "Metallic", "Roughness", "AO"
    };
    const char* kBlendNames[] = { "NONE", "NORMAL", "ADD", "MULTIPLY" };
}

void MaterialsInspectorPanel::Draw(bool* open)
{
    if (open && !*open) {
        return;
    }
    if (!ImGui::Begin("Materials Inspector", open)) {
        ImGui::End();
        return;
    }

    ObjectSelector* selector = Engine::GetInstance().GetObjectSelector().get();
    std::shared_ptr<Pivot3D> node = selector ? selector->GetSelectedObject() : nullptr;

    Mesh* mesh = dynamic_cast<Mesh*>(node.get());
    Material3D* material = mesh ? dynamic_cast<Material3D*>(mesh->GetMaterial().get()) : nullptr;

    if (!material) {
        ImGui::TextDisabled("Выбери меш с материалом");
        ImGui::End();
        return;
    }

    ImGui::Text("Lighting: %s", material->GetLightingTypeName().c_str());

    float roughnessScale = material->GetRoughnessScale();
    if (ImGui::DragFloat("Roughness scale", &roughnessScale, 0.01f, 0.0f, 4.0f)) {
        material->SetRoughnessScale(roughnessScale);
    }

    ImGui::SeparatorText("Filters");

    bool needRebuild = false; // изменения слота/блендинга меняют генерируемый код
    int index = 0;
    for (const auto& filter : material->GetFilters()) {
        ImGui::PushID(index++);

        if (ImGui::CollapsingHeader(filter->GetTypeName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            int slot = static_cast<int>(filter->GetSlot());
            if (ImGui::Combo("Slot", &slot, kSlotNames, IM_ARRAYSIZE(kSlotNames))) {
                filter->SetSlot(static_cast<Filter3D::FilterSlot>(slot));
                needRebuild = true;
            }

            int blend = static_cast<int>(filter->GetBlendMode());
            if (ImGui::Combo("Blend", &blend, kBlendNames, IM_ARRAYSIZE(kBlendNames))) {
                filter->SetBlendMode(static_cast<Filter3D::BlendMode>(blend));
                needRebuild = true;
            }

            if (auto* colorFilter = dynamic_cast<ColorFilter*>(filter.get())) {
                const unsigned int c = colorFilter->GetColor();
                float rgba[4] = {
                    ((c >> 24) & 0xff) / 255.0f,
                    ((c >> 16) & 0xff) / 255.0f,
                    ((c >> 8) & 0xff) / 255.0f,
                    (c & 0xff) / 255.0f,
                };
                if (ImGui::ColorEdit4("Color", rgba)) {
                    const unsigned int packed =
                        (static_cast<unsigned int>(rgba[0] * 255.0f) << 24) |
                        (static_cast<unsigned int>(rgba[1] * 255.0f) << 16) |
                        (static_cast<unsigned int>(rgba[2] * 255.0f) << 8) |
                        (static_cast<unsigned int>(rgba[3] * 255.0f));
                    colorFilter->SetColor(packed);
                }
            } else if (auto* texFilter = dynamic_cast<TextureFilterBase*>(filter.get())) {
                const auto& tex = texFilter->GetTexture();
                ImGui::TextWrapped("Texture: %s", tex ? tex->path.c_str() : "(none)");
            }
        }

        ImGui::PopID();
    }

    if (needRebuild) {
        material->Build(); // перегенерировать шейдер под новые слоты/блендинг
    }

    ImGui::End();
}
