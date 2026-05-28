#pragma once

#include <string>

#include "ILightingModel.h"
#include "GL/glew.h"
#include "render/RenderContext.h"
#include "render/UniformsLocationCache.h"

class PhongLightingModel : public ILightingModel
{
public:
    ~PhongLightingModel() override = default;
    const std::string& GetDeclarations() const override;
    const std::string& GetLightingCode() const override;
    void Bind(GLuint firstTextureUnit, const RenderContext& ctx) override;
    void OnProgramBuild(GLuint program) override;
private:
    static const std::string _declarationsCode;
    static const std::string _lightingCode;
    UniformsLocationCache _uniformCache;
};
