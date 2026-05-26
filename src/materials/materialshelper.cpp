#include "materialshelper.h"
#include "filters/Filter3d.h"
#include <iostream>

std::string MaterialsHelper::blendModeToStr(const BlendMode& blendMode)
{
    switch (blendMode)
    {
    case BlendMode::ADD:
        return "Add";

    case BlendMode::MULTIPLY:
        return "Multiply";

    case BlendMode::NORMAL:
        return "Normal";

    default:
        break;
    }
    return "";
}

BlendMode MaterialsHelper::stringToBlendMode(const std::string& blendMode)
{
    if (blendMode == "Add")
        return BlendMode::ADD;
    else if (blendMode == "Multiply")
        return BlendMode::MULTIPLY;
    else if (blendMode == "Normal")
        return BlendMode::NORMAL;

    return BlendMode::NONE;
}

std::string MaterialsHelper::cullFaceToStr(const CullFaceMode &cullFace)
{
    switch (cullFace)
    {
    case CullFaceMode::back:
        return "Back";

    case CullFaceMode::front:
        return "Front";

    case CullFaceMode::none:
        return "None";

    default:
        break;
    }
    return "";
}

CullFaceMode MaterialsHelper::stringToCullFace(const std::string &cullFace)
{
    if (cullFace == "Back")
        return CullFaceMode::back;
    else if (cullFace == "Front")
        return CullFaceMode::front;
    else if (cullFace == "None")
        return CullFaceMode::none;

    throw std::invalid_argument("illegal cullface mode");
}
