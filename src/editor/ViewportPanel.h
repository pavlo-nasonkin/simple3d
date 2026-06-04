#pragma once

#include <memory>

#include <imgui.h>
#include <ImGuizmo.h>

class Framebuffer;
class Scene3D;
class Camera;

// Панель «Viewport»: рендерит сцену в offscreen FBO и показывает его текстуру
// через ImGui::Image. Размер FBO и аспект камеры подстраиваются под размер панели.
// Поверх — гизмо ImGuizmo для выбранного узла (W/E/R — translate/rotate/scale).
class ViewportPanel
{
    std::unique_ptr<Framebuffer> _framebuffer;
    int _width = 0;
    int _height = 0;
    bool _hovered = false;
    ImGuizmo::OPERATION _gizmoOp = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE _gizmoMode = ImGuizmo::LOCAL;

public:
    ViewportPanel();
    ~ViewportPanel();

    void Draw(Scene3D* scene, Camera* camera);

    // true, если курсор над областью вьюпорта (тогда мышь отдаём камере).
    bool IsHovered() const { return _hovered; }
};
