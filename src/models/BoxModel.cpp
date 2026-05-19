#include "BoxModel.h"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "../Engine.h"
#include "../resources/TextureManager.h"
#include <string>
#include "../materials/ShaderFactory.h"
#include "../Device3D.h"
#include "../materials/Material3D.h"
#include "../materials/ColorMaterial.h"
#include "../materials/filters/ColorFilter.h"

BoxModel::BoxModel():
    Model()
{
}

BoxModel::~BoxModel()
{
}

void BoxModel::init()
{
    addChild(processMesh());
}

std::shared_ptr<Mesh> BoxModel::processMesh()
{
    auto mat = std::make_shared<Material3D>(ShaderFactory::getShader("../assets/shaders/shader.vs",
                                                          "../assets/shaders/defaultColorLight.fs"));

    auto colorFilter = std::make_shared<ColorFilter>();
    colorFilter->setColor(_color);
    colorFilter->setBlendMode(BlendMode::ADD);
    mat->addFilter(colorFilter);
    mat->build();
    auto mesh = std::make_shared<Mesh>(boxVertices, boxIndices, mat, std::vector<VertexBoneData>());
    mesh->setName(std::string("Box") + std::to_string(mesh->getId()));
    return mesh;
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

std::vector<Vertex> BoxModel::boxVertices = {
	{ glm::vec3(0.5f, 0.5f, -0.5f),		glm::vec3(0.0f, 0.0f, -1.0f),	glm::vec2(0.0f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, -0.5f),	glm::vec3(0.0f, 0.0f, -1.0f),	glm::vec2(1.0f, 0.0f) },
	{ glm::vec3(-0.5f, -0.5f, -0.5f),	glm::vec3(0.0f, 0.0f, -1.0f),	glm::vec2(1.0f, 1.0f) },
	{ glm::vec3(-0.5f, 0.5f, -0.5f),	glm::vec3(0.0f, 0.0f, -1.0f),	glm::vec2(0.0f, 1.0f) },

	{ glm::vec3(0.5f, 0.5f, 0.5f),		glm::vec3(0.0f, 0.0f, 1.0f),	glm::vec2(0.0f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, 0.5f),		glm::vec3(0.0f, 0.0f, 1.0f),	glm::vec2(1.0f, 0.0f) },
	{ glm::vec3(-0.5f, -0.5f, 0.5f),	glm::vec3(0.0f, 0.0f, 1.0f),	glm::vec2(1.0f, 1.0f) },
	{ glm::vec3(-0.5f, 0.5f, 0.5f),		glm::vec3(0.0f, 0.0f, 1.0f),	glm::vec2(0.0f, 1.0f) },

	{ glm::vec3(0.5f,	0.5f, 0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(0.0f, 0.0f) },
	{ glm::vec3(0.5f,	0.5f, -0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(1.0f, 0.0f) },
	{ glm::vec3(-0.5f,	0.5f, -0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(1.0f, 1.0f) },
	{ glm::vec3(-0.5f,	0.5f, 0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(0.0f, 1.0f) },

	{ glm::vec3(0.5f,	-0.5f, 0.5f),	glm::vec3(0.0f, -1.0f, 0.0f),	glm::vec2(0.0f, 0.0f) },
	{ glm::vec3(0.5f,	-0.5f, -0.5f),	glm::vec3(0.0f, -1.0f, 0.0f),	glm::vec2(1.0f, 0.0f) },
	{ glm::vec3(-0.5f,	-0.5f, -0.5f),	glm::vec3(0.0f, -1.0f, 0.0f),	glm::vec2(1.0f, 1.0f) },
	{ glm::vec3(-0.5f,	-0.5f, 0.5f),	glm::vec3(0.0f, -1.0f, 0.0f),	glm::vec2(0.0f, 1.0f) },

	{ glm::vec3(0.5f, 0.5f, 0.5f),		glm::vec3(1.0f, 0.0f, 0.0f),	glm::vec2(0.0f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, 0.5f),		glm::vec3(1.0f, 0.0f, 0.0f),	glm::vec2(1.0f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, -0.5f),	glm::vec3(1.0f, 0.0f, 0.0f),	glm::vec2(1.0f, 1.0f) },
	{ glm::vec3(0.5f, 0.5f, -0.5f),		glm::vec3(1.0f, 0.0f, 0.0f),	glm::vec2(0.0f, 1.0f) },

	{ glm::vec3(-0.5f, 0.5f, 0.5f),		glm::vec3(-1.0f, 0.0f, 0.0f),	glm::vec2(0.0f, 0.0f) },
	{ glm::vec3(-0.5f, -0.5f, 0.5f),	glm::vec3(-1.0f, 0.0f, 0.0f),	glm::vec2(1.0f, 0.0f) },
	{ glm::vec3(-0.5f, -0.5f, -0.5f),	glm::vec3(-1.0f, 0.0f, 0.0f),	glm::vec2(1.0f, 1.0f) },
	{ glm::vec3(-0.5f, 0.5f, -0.5f),	glm::vec3(-1.0f, 0.0f, 0.0f),	glm::vec2(0.0f, 1.0f) },
};
