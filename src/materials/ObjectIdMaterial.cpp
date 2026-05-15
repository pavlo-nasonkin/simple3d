#include "ObjectIdMaterial.h"
#include "GLEWImporter.h"
#include "Shader.h"
#include "Device3D.h"
#include "Mesh.h"
#include "Pivot3D.h"

ObjectIdMaterial::ObjectIdMaterial(std::shared_ptr<Shader> shader)
	:MaterialBase(shader)
{
}

ObjectIdMaterial::~ObjectIdMaterial()
{

}

void ObjectIdMaterial::bind(const Mesh* mesh/*=nullptr*/)
{
	MaterialBase::bind(mesh);

	

	GLint modelLoc = glGetUniformLocation(_shader->Program, "model");
	GLint viewLoc = glGetUniformLocation(_shader->Program, "view");
	GLint projectionLoc = glGetUniformLocation(_shader->Program, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, Device3D::model);
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, Device3D::view);
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, Device3D::projection);

	//Bind id
	GLint colorLoc = glGetUniformLocation(_shader->Program, "uColor");
	unsigned char pixel[4];
    unsigned int id = mesh->getId();
	pixel[0] = id & 0xff;
	pixel[1] = (id >> 8) & 0xff;
	pixel[2] = (id >> 16) & 0xff;
	pixel[3] = (id >> 24) & 0xff;
	glUniform4f(colorLoc, pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, 1.0);

}
