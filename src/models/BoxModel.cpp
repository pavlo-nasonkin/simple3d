#include "BoxModel.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "Engine.h"
#include <string>

#include "lighting/UnlitLightingModel.h"
#include "materials/Material3D.h"
#include "materials/filters/ColorFilter.h"
#include "render/VertexLayoutPresets.h"
#include "render/Geometry.h"
#include "render/GeometryRegistry.h"
#include "render/MeshRenderer.h"
#include "utils/AssetPaths.h"

BoxModel::BoxModel()
{
}

void BoxModel::Init()
{
    AddChild(ProcessMesh());
}

std::shared_ptr<Pivot3D> BoxModel::ProcessMesh()
{
	auto mat = std::make_shared<Material3D>(AssetPaths::Resolve("shaders/shader.vsh"),
														  AssetPaths::Resolve("shaders/defaultColorLight.fsh"));
	// mat->SetLightingModel(std::make_unique<UnlitLightingModel>());
	_colorFilter = std::make_shared<ColorFilter>();
	_colorFilter->SetColor(_color);
	_colorFilter->SetBlendMode(Filter3D::BlendMode::MULTIPLY);
    mat->AddFilter(_colorFilter);
    mat->Build();
    auto geometry = Engine::GetInstance().GetGeometryRegistry().GetOrCreate(
        "primitive:box",
        [] { return Geometry(VertexLayouts::Standard(), boxVertices, boxIndices); });
    auto node = MeshRenderer::MakeNode(geometry, mat);
    node->SetName(std::string("Box") + std::to_string(node->GetId()));
    return node;
}

void BoxModel::SetColor(unsigned int color) {
	_color = color;
	if (_colorFilter) {
		_colorFilter->SetColor(color);
	}
}

std::vector<GLuint> BoxModel::boxIndices = {
	0,1,3,
	1,2,3,

	4,7,5,
	5,7,6,

	8,9,11,
	9,10,11,

	12,15,13,
	13,15,14,

	16,17,18,
	16,18,19,

	20,22,21,
	20,23,22,
};

std::vector<VertexTypes::Vertex> BoxModel::boxVertices = {
	// -Z face: T = (0, -1, 0)
	{ glm::vec3(0.5f, 0.5f, -0.5f),		glm::vec3(0.0f, 0.0f, -1.0f),	glm::vec2(0.0f, 0.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, -0.5f),	glm::vec3(0.0f, 0.0f, -1.0f),	glm::vec2(1.0f, 0.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(-0.5f, -0.5f, -0.5f),	glm::vec3(0.0f, 0.0f, -1.0f),	glm::vec2(1.0f, 1.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(-0.5f, 0.5f, -0.5f),	glm::vec3(0.0f, 0.0f, -1.0f),	glm::vec2(0.0f, 1.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },

	// +Z face: T = (0, -1, 0)
	{ glm::vec3(0.5f, 0.5f, 0.5f),		glm::vec3(0.0f, 0.0f, 1.0f),	glm::vec2(0.0f, 0.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, 0.5f),		glm::vec3(0.0f, 0.0f, 1.0f),	glm::vec2(1.0f, 0.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(-0.5f, -0.5f, 0.5f),	glm::vec3(0.0f, 0.0f, 1.0f),	glm::vec2(1.0f, 1.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(-0.5f, 0.5f, 0.5f),		glm::vec3(0.0f, 0.0f, 1.0f),	glm::vec2(0.0f, 1.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },

	// +Y face: T = (0, 0, -1)
	{ glm::vec3(0.5f,	0.5f, 0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(0.0f, 0.0f),	glm::vec3(0.0f, 0.0f, -1.0f) },
	{ glm::vec3(0.5f,	0.5f, -0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(1.0f, 0.0f),	glm::vec3(0.0f, 0.0f, -1.0f) },
	{ glm::vec3(-0.5f,	0.5f, -0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(1.0f, 1.0f),	glm::vec3(0.0f, 0.0f, -1.0f) },
	{ glm::vec3(-0.5f,	0.5f, 0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(0.0f, 1.0f),	glm::vec3(0.0f, 0.0f, -1.0f) },

	// -Y face: T = (0, 0, -1)
	{ glm::vec3(0.5f,	-0.5f, 0.5f),	glm::vec3(0.0f, -1.0f, 0.0f),	glm::vec2(0.0f, 0.0f),	glm::vec3(0.0f, 0.0f, -1.0f) },
	{ glm::vec3(0.5f,	-0.5f, -0.5f),	glm::vec3(0.0f, -1.0f, 0.0f),	glm::vec2(1.0f, 0.0f),	glm::vec3(0.0f, 0.0f, -1.0f) },
	{ glm::vec3(-0.5f,	-0.5f, -0.5f),	glm::vec3(0.0f, -1.0f, 0.0f),	glm::vec2(1.0f, 1.0f),	glm::vec3(0.0f, 0.0f, -1.0f) },
	{ glm::vec3(-0.5f,	-0.5f, 0.5f),	glm::vec3(0.0f, -1.0f, 0.0f),	glm::vec2(0.0f, 1.0f),	glm::vec3(0.0f, 0.0f, -1.0f) },

	// +X face: T = (0, -1, 0)
	{ glm::vec3(0.5f, 0.5f, 0.5f),		glm::vec3(1.0f, 0.0f, 0.0f),	glm::vec2(0.0f, 0.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, 0.5f),		glm::vec3(1.0f, 0.0f, 0.0f),	glm::vec2(1.0f, 0.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, -0.5f),	glm::vec3(1.0f, 0.0f, 0.0f),	glm::vec2(1.0f, 1.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(0.5f, 0.5f, -0.5f),		glm::vec3(1.0f, 0.0f, 0.0f),	glm::vec2(0.0f, 1.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },

	// -X face: T = (0, -1, 0)
	{ glm::vec3(-0.5f, 0.5f, 0.5f),		glm::vec3(-1.0f, 0.0f, 0.0f),	glm::vec2(0.0f, 0.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(-0.5f, -0.5f, 0.5f),	glm::vec3(-1.0f, 0.0f, 0.0f),	glm::vec2(1.0f, 0.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(-0.5f, -0.5f, -0.5f),	glm::vec3(-1.0f, 0.0f, 0.0f),	glm::vec2(1.0f, 1.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
	{ glm::vec3(-0.5f, 0.5f, -0.5f),	glm::vec3(-1.0f, 0.0f, 0.0f),	glm::vec2(0.0f, 1.0f),	glm::vec3(0.0f, -1.0f, 0.0f) },
};
