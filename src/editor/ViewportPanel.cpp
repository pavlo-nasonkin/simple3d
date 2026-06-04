#include "ViewportPanel.h"

#include <GL/glew.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstdint>
#include <memory>

#include "render/Framebuffer.h"
#include "render/RenderContext.h"
#include "Scene3D.h"
#include "Pivot3D.h"
#include "camera/Camera.h"
#include "Engine.h"
#include "object_selector/ObjectSelector.h"

ViewportPanel::ViewportPanel() = default;
ViewportPanel::~ViewportPanel() = default;

void ViewportPanel::Draw(Scene3D* scene, Camera* camera)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    // NoMove — чтобы drag ЛКМ по 3D-сцене не таскал само окно; скролл тоже не нужен.
    ImGui::Begin("Viewport", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const int w = std::max(1, static_cast<int>(avail.x));
    const int h = std::max(1, static_cast<int>(avail.y));

    if (!_framebuffer) {
        _framebuffer = std::make_unique<Framebuffer>(w, h);
    } else {
        _framebuffer->Resize(w, h);
    }

    // Аспект/размер камеры следуют за панелью, а не за окном.
    if (w != _width || h != _height) {
        _width = w;
        _height = h;
        camera->SetScreenWidth(static_cast<float>(w));
        camera->SetScreenHeight(static_cast<float>(h));
        camera->buildProjectionMatrix(45.0f, 0.1f, 100.0f);
    }

    // Рендер сцены в FBO. Scene3D::Render сам сохраняет/восстанавливает FBO в
    // shadow-pass, так что наш target остаётся активным для основного прохода.
    _framebuffer->Bind();
    RenderContext ctx;
    ctx.camera = camera;
    ctx.view = camera->GetViewMatrix();
    ctx.projection = camera->getProjectionMatrix();
    ctx.scene3D = scene;
    scene->Render(ctx);
    Framebuffer::Unbind();

    // GL-текстура: origin внизу-слева → переворачиваем V (uv0=(0,1), uv1=(1,0)).
    const ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(_framebuffer->ColorTexture()));
    const ImVec2 imageMin = ImGui::GetCursorScreenPos();
    ImGui::Image(texId, avail, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

    _hovered = ImGui::IsWindowHovered();

    ObjectSelector* selector = Engine::GetInstance().GetObjectSelector().get();
    std::shared_ptr<Pivot3D> selected = selector ? selector->GetSelectedObject() : nullptr;

    // ── Гизмо ImGuizmo над выбранным узлом ──
    if (selected) {
        if (_hovered) {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) _gizmoOp = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) _gizmoOp = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) _gizmoOp = ImGuizmo::SCALE;
        }

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(imageMin.x, imageMin.y, static_cast<float>(_width), static_cast<float>(_height));

        glm::mat4 view = camera->GetViewMatrix();
        glm::mat4 proj = camera->getProjectionMatrix();

        const glm::vec3 pos = *selected->GetPosition();
        const glm::vec3 rotDeg = glm::degrees(*selected->GetRotation());
        const glm::vec3 scl = *selected->GetScale();
        float t[3] = { pos.x, pos.y, pos.z };
        float r[3] = { rotDeg.x, rotDeg.y, rotDeg.z };
        float s[3] = { scl.x, scl.y, scl.z };

        glm::mat4 model(1.0f);
        ImGuizmo::RecomposeMatrixFromComponents(t, r, s, glm::value_ptr(model));

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
                                 _gizmoOp, _gizmoMode, glm::value_ptr(model))) {
            float nt[3], nr[3], ns[3];
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), nt, nr, ns);
            selected->SetPosition(nt[0], nt[1], nt[2]);
            const glm::vec3 rad = glm::radians(glm::vec3(nr[0], nr[1], nr[2]));
            selected->SetRotation(rad.x, rad.y, rad.z);
            selected->SetScale(ns[0], ns[1], ns[2]);
        }
    }

    // ЛКМ по сцене — пикинг (если не взаимодействуем с гизмо).
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
        && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing()) {
        const ImVec2 mouse = ImGui::GetMousePos();
        const int localX = static_cast<int>(mouse.x - imageMin.x);
        const int localY = static_cast<int>(mouse.y - imageMin.y);
        if (localX >= 0 && localY >= 0 && localX < _width && localY < _height) {
            if (selector) {
                selector->PickAt(localX, localY, _width, _height);
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
