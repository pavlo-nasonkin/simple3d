#include "PrimitiveModel.h"

#include <string>

#include "Engine.h"
#include "materials/Material3D.h"
#include "materials/filters/ColorFilter.h"
#include "render/VertexLayoutPresets.h"
#include "render/Geometry.h"
#include "render/GeometryRegistry.h"
#include "render/MeshRenderer.h"
#include "utils/AssetPaths.h"

void PrimitiveModel::Init()
{
    AddChild(ProcessMesh());
}

void PrimitiveModel::SetColor(unsigned int color)
{
    _color = color;
    if (_colorFilter) {
        _colorFilter->SetColor(color);
    }
}

std::shared_ptr<Pivot3D> PrimitiveModel::ProcessMesh()
{
    auto mat = std::make_shared<Material3D>(AssetPaths::Resolve("shaders/shader.vsh"),
                                            AssetPaths::Resolve("shaders/defaultColorLight.fsh"));
    _colorFilter = std::make_shared<ColorFilter>();
    _colorFilter->SetColor(_color);
    _colorFilter->SetBlendMode(Filter3D::BlendMode::MULTIPLY);
    mat->AddFilter(_colorFilter);
    mat->SetCullFace(CullFaceMode::none); // двусторонние примитивы — не зависим от winding
    mat->Build();

    auto geometry = Engine::GetInstance().GetGeometryRegistry().GetOrCreate(
        GeometryKey(),
        [this] {
            std::vector<VertexTypes::Vertex> vertices;
            std::vector<GLuint> indices;
            GenerateGeometry(vertices, indices);
            return Geometry(VertexLayouts::Standard(), vertices, indices);
        });

    return MeshRenderer::MakeNode(geometry, mat, GeometryKey());
}
