#pragma once

#include <string>

#include "ILightingModel.h"
#include "GL/glew.h"
#include "render/RenderContext.h"

class UnlitLightingModel : public ILightingModel
{
public:
    ~UnlitLightingModel() override = default;
    const std::string& GetDeclarations() const override;
    const std::string& GetLightingCode() const override;
    void Bind(GLuint firstTextureUnit, const RenderContext& ctx) override {}
private:
    static const std::string _declarationsCode;
    static const std::string _lightingCode;
};
