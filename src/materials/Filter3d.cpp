#include "Filter3d.h"

unsigned int Filter3D::_filtersIdCounter = 0;

BlendMode Filter3D::blendMode() const
{
    return _blendMode;
}

void Filter3D::setBlendMode(const BlendMode &blendMode)
{
    _blendMode = blendMode;
}

std::string Filter3D::generatedUniqueName() const
{
    return _generatedUniqueName;
}

Filter3D::Filter3D()
    :_blendMode(BlendMode::ADD),
      _type(FilterType::FRAGMENT),
      _id(Filter3D::_filtersIdCounter)
{
    Filter3D::_filtersIdCounter++;
}

Filter3D::~Filter3D()
{
    
}

void Filter3D::bind(GLuint /*program*/)
{

}

std::string Filter3D::name() const
{
    return _name;
}

void Filter3D::setName(const std::string& name)
{
    _name = name;
}

std::string Filter3D::code() const
{
    return _code;
}

void Filter3D::setCode(const std::string& code)
{
    _code = code;
}

FilterType Filter3D::type() const
{
    return _type;
}
