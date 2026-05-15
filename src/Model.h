#pragma once
#ifndef Model_h__
#define Model_h__
#include "Pivot3D.h"
#include "materials/MaterialBase.h"

class Model: public Pivot3D
{
public:
	Model();
	~Model() override;
    void render(std::shared_ptr<MaterialBase> shader = nullptr) override;
protected:
	
private:
};

#endif // Model_h__
