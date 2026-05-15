#pragma once
#ifndef IObjectSelectorListener_h__
#define IObjectSelectorListener_h__

#include <memory>
#include "Pivot3D.h"

class IObjectSelectorListener
{
public:
    virtual void handleSelectedObject(std::shared_ptr<Pivot3D> /*pivot*/) {}
};

#endif // IObjectSelectorListener_h__
