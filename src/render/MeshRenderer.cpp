#include "MeshRenderer.h"

#include <cassert>
#include <utility>

#include "Pivot3D.h"
#include "render/RenderContext.h"
#include "render/Geometry.h"
#include "materials/MaterialBase.h"

MeshRenderer::MeshRenderer(std::shared_ptr<Geometry> geometry, std::shared_ptr<MaterialBase> material)
    : _geometry(std::move(geometry)), _material(std::move(material))
{
    assert(_material && "MeshRenderer requires a non-null material");
}

void MeshRenderer::Draw(const RenderContext& ctx, const Pivot3D* owner, MaterialBase* overrideMaterial) const
{
    MaterialBase* material = overrideMaterial ? overrideMaterial : _material.get();
    if (!material || !_geometry || _geometry->IndicesCount() == 0) {
        return;
    }

    RenderContext local = ctx;
    local.model = ctx.model * _nodeMatrix; // pre-PRS transform поверх world-матрицы узла

    material->Bind(local, owner);
    _geometry->Draw();
    material->Unbind();
}

std::shared_ptr<Pivot3D> MeshRenderer::MakeNode(std::shared_ptr<Geometry> geometry,
                                                std::shared_ptr<MaterialBase> material,
                                                const std::string& name)
{
    auto node = std::make_shared<Pivot3D>();
    node->SetRenderer(std::make_unique<MeshRenderer>(std::move(geometry), std::move(material)));
    node->SetName(name);
    return node;
}
