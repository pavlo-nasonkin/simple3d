#include "ColorFilter.h"
#include "utils/StringUtils.h"

std::string ColorFilter::_colorFilterCode =
        "uniform vec4 uColor{id};\n"
        "vec4 colorFilter{id}()\n"
        "{\n"
        "    return uColor{id};\n"
        "}\n";

unsigned int ColorFilter::color() const
{
    return _color;
}

void ColorFilter::setColor(unsigned int color)
{
    _color = color;
}

ColorFilter::ColorFilter()
    :Filter3D(),
      _color(0xFF000000),
      _uniformName()
{
    _code = _colorFilterCode;
    _name = "colorFilter";
    const std::string idStr = std::to_string(_id);
    _generatedUniqueName = _name + idStr;
    _uniformName = "uColor" + idStr;
    StringUtils::replace(_code, "{id}", idStr);
}

ColorFilter::~ColorFilter()
{

}

void ColorFilter::bind(GLuint program)
{
    Filter3D::bind(program);
    //TODO make _uniformName c_str by default
    GLint colorLoc = glGetUniformLocation(program, _uniformName.c_str());
    unsigned char pixel[4];

    pixel[3] = _color & 0xff; //a
    pixel[2] = (_color >> 8) & 0xff; //b
    pixel[1] = (_color >> 16) & 0xff; //g
    pixel[0] = (_color >> 24) & 0xff; //r
    glUniform4f(colorLoc, pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, 1.0);
}
