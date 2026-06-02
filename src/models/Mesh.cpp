#include "Mesh.h"

#include <cassert>
#include <utility>

#include "materials/MaterialBase.h"
#include "render/Geometry.h"

Mesh::Mesh(std::shared_ptr<Geometry> geometry, std::shared_ptr<MaterialBase> material) :
    _geometry(std::move(geometry)),
    _material(std::move(material))
{
    assert(_material && "Mesh requires a non-null material");
    _name = "Mesh";
}

void Mesh::Render(const RenderContext &ctx, MaterialBase* material)
{
    if (ctx.shadowPass && !_castShadows) {
        return;
    }

    RenderContext context = ctx;
    context.model = ctx.model * LocalMatrix();
    if (material == nullptr) {
        material = _material.get();
    }

    if (_geometry && _geometry->IndicesCount() > 0) {
        material->Bind(context, this);
        _geometry->Draw();
        material->Unbind();
    }

    for (auto& child : _children) {
        child->Render(context, material);
    }
}

glm::mat4 Mesh::LocalMatrix() const
{
    // Исходный transform сцены (напр. COLLADA Z_UP -> Y_UP) поверх пользовательского PRS.
    return _nodeMatrix * Pivot3D::LocalMatrix();
}
