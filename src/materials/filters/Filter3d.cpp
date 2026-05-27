#include "Filter3d.h"


void Filter3D::Init()
{
    _code = GetBaseFilterCode();
    _generatedUniqueName = _name + std::to_string(_id);
}