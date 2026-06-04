#pragma once

#include <string>

#include "ILightingModel.h"
#include "GL/glew.h"
#include "render/RenderContext.h"
#include "render/UniformsLocationCache.h"

class PBRLightingModel : public ILightingModel
{
public:
    ~PBRLightingModel() override = default;
    const std::string& GetDeclarations() const override;
    const std::string& GetLightingCode() const override;
    void Bind(GLuint firstTextureUnit, const RenderContext& ctx) override;
    void OnProgramBuild(GLuint program) override;
    unsigned int GetTextureUnitCount() const override { return 4; } // irradiance + prefilter + brdfLUT + shadowMap
    std::string GetTypeName() const override { return "PBR"; }
private:
    static const std::string _declarationsCode;
    static const std::string _lightingCode;
    UniformsLocationCache _uniformCache;
};
