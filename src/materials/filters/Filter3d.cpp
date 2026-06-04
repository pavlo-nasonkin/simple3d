#include "Filter3d.h"

#include "FilterData.h"


void Filter3D::Init()
{
    _code = GetBaseFilterCode();
    _generatedUniqueName = _name + std::to_string(_id);
}

FilterData Filter3D::Serialize() const
{
    FilterData data;
    data.type = GetTypeName();
    data.slot = _slot;
    data.blend = _blendMode;
    return data;
}