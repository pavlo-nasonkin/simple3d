#include "ColorMaterial.h"
#include "GLEWImporter.h"
#include "Shader.h"
#include "Scene3D.h"

ColorMaterial::ColorMaterial(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
	:MaterialBase(vertexShaderPath, fragmentShaderPath)
{
}

void ColorMaterial::Bind(const RenderContext& ctx, const Mesh* mesh/*=nullptr*/)
{
	MaterialBase::Bind(ctx, mesh);

	GLint colorLoc = _uniformCache.GetUniformLocation("uColor");
	unsigned char pixel[4];
	unsigned int id = 0x999999;
	pixel[0] = id & 0xff;
	pixel[1] = (id >> 8) & 0xff;
	pixel[2] = (id >> 16) & 0xff;
	pixel[3] = (id >> 24) & 0xff;
	glUniform4f(colorLoc, pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, 1.0);
}

