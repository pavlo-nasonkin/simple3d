#include "ObjectIdMaterial.h"
#include "GLEWImporter.h"
#include "Shader.h"
#include "models/Mesh.h"
#include "Pivot3D.h"
#include "glm/gtc/type_ptr.hpp"

ObjectIdMaterial::ObjectIdMaterial(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
	:MaterialBase(vertexShaderPath, fragmentShaderPath)
{

}

void ObjectIdMaterial::Bind(const RenderContext& ctx, const Mesh* mesh/*=nullptr*/)
{
	MaterialBase::Bind(ctx, mesh);

	GLint modelLoc = glGetUniformLocation(_shader->GetProgram(), "model");
	GLint viewLoc = glGetUniformLocation(_shader->GetProgram(), "view");
	GLint projectionLoc = glGetUniformLocation(_shader->GetProgram(), "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ctx.model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(ctx.view));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(ctx.projection));

	//Bind id
	GLint colorLoc = glGetUniformLocation(_shader->GetProgram(), "uColor");
	unsigned char pixel[4];
    unsigned int id = mesh->GetId();
	pixel[0] = id & 0xff;
	pixel[1] = (id >> 8) & 0xff;
	pixel[2] = (id >> 16) & 0xff;
	pixel[3] = (id >> 24) & 0xff;
	glUniform4f(colorLoc, pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, 1.0);
}
