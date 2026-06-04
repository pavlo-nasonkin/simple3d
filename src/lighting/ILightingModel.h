#pragma once

#include <string>

#include "GL/glew.h"

class RenderContext;

class ILightingModel
{
public:
    virtual ~ILightingModel() = default;

    // GLSL: struct Light, struct Material, uniform Light light; uniform Material material;
    // плюс in-переменные, которые модель ждёт от vertex-шейдера (FragPos, Normal, TBN).
    virtual const std::string& GetDeclarations() const = 0;

    virtual const std::string& GetLightingCode() const = 0;

    virtual void Bind(GLuint firstTextureUnit, const RenderContext& ctx) = 0;
    virtual void Unbind(GLuint firstTextureUnit) {}

    virtual unsigned int GetTextureUnitCount() const { return 0; }
    virtual void OnProgramBuild(GLuint program) {}

    // Имя типа для сериализации материала ("Phong"/"PBR"/"Unlit").
    virtual std::string GetTypeName() const { return "Unlit"; }
};
