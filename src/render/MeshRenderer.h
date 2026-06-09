#pragma once

#include <memory>
#include <string>

#include <glm/glm.hpp>

struct RenderContext;
class Pivot3D;
class Geometry;
class MaterialBase;

// Рендер-компонент-ДАННЫЕ узла: ссылается на (шарящуюся) Geometry + материал и
// опциональную pre-PRS nodeMatrix (накопленный aiNode transform). Сам не узел и не
// тикается — его рисует Pivot3D::Render. Заменяет прежний класс-узел Mesh
// (DEVELOP_PLAN 9.12): теперь «меш» = обычный Pivot3D + MeshRenderer.
class MeshRenderer
{
public:
    MeshRenderer(std::shared_ptr<Geometry> geometry, std::shared_ptr<MaterialBase> material);

    // Рисует геометрию материалом (override имеет приоритет — depth/id-pass).
    // model = ctx.model * nodeMatrix; owner передаётся материалу (id, receiveShadows).
    void Draw(const RenderContext& ctx, const Pivot3D* owner, MaterialBase* overrideMaterial) const;

    const std::shared_ptr<MaterialBase>& GetMaterial() const { return _material; }
    void SetMaterial(const std::shared_ptr<MaterialBase>& material) { _material = material; }
    const std::shared_ptr<Geometry>& GetGeometry() const { return _geometry; }

    const glm::mat4& GetNodeMatrix() const { return _nodeMatrix; }
    void SetNodeMatrix(const glm::mat4& m) { _nodeMatrix = m; }

    // Узел Pivot3D с прикреплённым MeshRenderer (замена конструктора Mesh).
    static std::shared_ptr<Pivot3D> MakeNode(std::shared_ptr<Geometry> geometry,
                                             std::shared_ptr<MaterialBase> material,
                                             const std::string& name = "Mesh");

private:
    std::shared_ptr<Geometry> _geometry;
    std::shared_ptr<MaterialBase> _material;
    glm::mat4 _nodeMatrix = glm::mat4(1.0f);
};
