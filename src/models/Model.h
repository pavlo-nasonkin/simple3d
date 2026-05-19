#pragma once

#include "../Pivot3D.h"
#include "../materials/MaterialBase.h"

class Model: public Pivot3D
{
public:
	Model();
	~Model() override = default;
};