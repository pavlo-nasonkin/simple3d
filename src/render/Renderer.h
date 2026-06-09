#pragma once

struct RenderContext;
class Scene3D;
class MaterialBase;

// Оркестратор проходов кадра: shadow → clear → main → skybox. Данные/ресурсы (свет,
// карта теней, skybox) читает из Scene3D; сам граф рисует обходом Pivot3D. Вынесен из
// Scene3D, чтобы добавление новых проходов не утолщало сцену (REFACTORING_PLAN 8.3).
class Renderer
{
public:
    // overrideMaterial != nullptr — спец-проход (напр. id-pass пикинга): без теней и skybox.
    static void RenderScene(Scene3D& scene, const RenderContext& ctx, MaterialBase* overrideMaterial = nullptr);
};
