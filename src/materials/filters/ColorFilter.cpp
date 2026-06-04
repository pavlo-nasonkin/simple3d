#include "ColorFilter.h"
#include "utils/StringUtils.h"
#include "FilterData.h"

FilterData ColorFilter::Serialize() const
{
    FilterData data = Filter3D::Serialize();
    data.color = _color;
    return data;
}

std::string ColorFilter::_colorFilterCode =
        "uniform vec4 uColor{uniform_id};\n"
        "vec4 colorFilter{id}()\n"
        "{\n"
        "    return uColor{uniform_id};\n"
        "}\n";

ColorFilter::ColorFilter()
    :Filter3D(),
      _color(0xFF000000)
{
    _name = "colorFilter";
    _resultType = ResultType::VEC4;
}

void ColorFilter::Init() {
    Filter3D::Init();

    const std::string idStr = std::to_string(_id);
    const std::string uniformIdStr = std::to_string(_nextUniformId);
    _uniformName = "uColor" + uniformIdStr;
    StringUtils::replace(_code, "{id}", idStr);
    StringUtils::replace(_code, "{uniform_id}", uniformIdStr);
}

void ColorFilter::Bind(GLuint program, GLuint firstTextureUnit)
{
    Filter3D::Bind(program, firstTextureUnit);

    GLint colorLoc = glGetUniformLocation(program, _uniformName.c_str());
    unsigned char pixel[4];

    pixel[3] = _color & 0xff; //a
    pixel[2] = (_color >> 8) & 0xff; //b
    pixel[1] = (_color >> 16) & 0xff; //g
    pixel[0] = (_color >> 24) & 0xff; //r
    glUniform4f(colorLoc, pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, 1.0);
}
