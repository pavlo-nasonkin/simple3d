#include "ObjectIdMaterial.h"
#include "GLEWImporter.h"
#include "Pivot3D.h"
#include "glm/gtc/type_ptr.hpp"

ObjectIdMaterial::ObjectIdMaterial(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
	:MaterialBase(vertexShaderPath, fragmentShaderPath)
{

}

void ObjectIdMaterial::Bind(const RenderContext& ctx, const Pivot3D* node/*=nullptr*/)
{
	MaterialBase::Bind(ctx, node);

	//Bind id
	GLint colorLoc = _uniformCache.GetUniformLocation("uColor");
	unsigned char pixel[4];
    unsigned int id = node ? node->GetId() : 0u;
	pixel[0] = id & 0xff;
	pixel[1] = (id >> 8) & 0xff;
	pixel[2] = (id >> 16) & 0xff;
	pixel[3] = (id >> 24) & 0xff;
	glUniform4f(colorLoc, pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, 1.0);
}
