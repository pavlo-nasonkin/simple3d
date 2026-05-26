#include "ColorMaterial.h"
#include "GLEWImporter.h"
#include "Shader.h"
#include "camera/Camera.h"
#include "Scene3D.h"

ColorMaterial::ColorMaterial(const std::shared_ptr<Shader>& shader)
	:MaterialBase(shader)
{
}

void ColorMaterial::Bind(const RenderContext& ctx, const Mesh* mesh/*=nullptr*/)
{
	MaterialBase::Bind(ctx, mesh);


	GLint modelLoc = glGetUniformLocation(_shader->Program, "model");
	GLint viewLoc = glGetUniformLocation(_shader->Program, "view");
	GLint projectionLoc = glGetUniformLocation(_shader->Program, "projection");
	GLint viewPosLoc = glGetUniformLocation(_shader->Program, "viewPos");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ctx.model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(ctx.view));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(ctx.projection));
	glUniform3f(viewPosLoc, ctx.camera->Position.x, ctx.camera->Position.y, ctx.camera->Position.z);

	GLint lightPositionLoc = glGetUniformLocation(_shader->Program, "light.position");
	GLint lightAmbientLoc = glGetUniformLocation(_shader->Program, "light.ambient");
	GLint lightDiffuseLoc = glGetUniformLocation(_shader->Program, "light.diffuse");
	GLint lightSpecularLoc = glGetUniformLocation(_shader->Program, "light.specular");
	const glm::vec3* lightAmbient = ctx.scene3D->getLightAmbient();
	const glm::vec3* lightDiffuce = ctx.scene3D->getLightDiffuse();
	const glm::vec3* lightSpecular = ctx.scene3D->getLightSpecular();
	const glm::vec3* lightPos = ctx.scene3D->getLightPosition();

	glUniform3f(lightAmbientLoc, lightAmbient->x, lightAmbient->y, lightAmbient->z);
	glUniform3f(lightDiffuseLoc, lightDiffuce->x, lightDiffuce->y, lightDiffuce->z); // Let's darken the light a bit to fit the scene
	glUniform3f(lightSpecularLoc, lightSpecular->x, lightSpecular->y, lightSpecular->z);
	glUniform3f(lightPositionLoc, lightPos->x, lightPos->y, lightPos->z);

	//Bind id
	GLint colorLoc = glGetUniformLocation(_shader->Program, "uColor");
	unsigned char pixel[4];
	unsigned int id = 0x999999;
	pixel[0] = id & 0xff;
	pixel[1] = (id >> 8) & 0xff;
	pixel[2] = (id >> 16) & 0xff;
	pixel[3] = (id >> 24) & 0xff;
	glUniform4f(colorLoc, pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, 1.0);

	glUniform1f(glGetUniformLocation(_shader->Program, "material.shininess"), 16.0f);

}

