#ifndef MATERIALSHELPER_H
#define MATERIALSHELPER_H

#include <string>
#include "filters/Filter3d.h"
#include "MaterialBase.h"

class MaterialsHelper
{
public:
    static std::string blendModeToStr(const BlendMode& blendMode);
    static BlendMode stringToBlendMode(const std::string& blendMode);
    static std::string cullFaceToStr(const CullFaceMode& cullFace);
    static CullFaceMode stringToCullFace(const std::string& cullFace);
};

#endif // MATERIALSHELPER_H
