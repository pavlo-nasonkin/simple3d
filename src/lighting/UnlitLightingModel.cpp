#include "UnlitLightingModel.h"

#include "Scene3D.h"

const std::string UnlitLightingModel::_declarationsCode = "";

const std::string UnlitLightingModel::_lightingCode =
    "color = vec4(BASE_COLOR + EMISSIVE, 1.0);\n";

const std::string& UnlitLightingModel::GetDeclarations() const
{
    return _declarationsCode;
}

const std::string & UnlitLightingModel::GetLightingCode() const {
    return _lightingCode;
}