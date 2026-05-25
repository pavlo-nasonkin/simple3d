#pragma once

#include <memory>
#include "Pivot3D.h"

class IObjectSelectorListener
{
public:
    virtual ~IObjectSelectorListener() = default;

    virtual void handleSelectedObject(std::shared_ptr<Pivot3D> /*pivot*/) {}
};