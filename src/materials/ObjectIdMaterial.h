#pragma once
#ifndef ObjectIdMaterial_h__
#define ObjectIdMaterial_h__
#include "MaterialBase.h"
class Mesh;

class ObjectIdMaterial: public MaterialBase
{
public:
    ObjectIdMaterial(std::shared_ptr<Shader> shader);
	~ObjectIdMaterial();
	void bind(const Mesh* mesh = nullptr) override;
protected:
	
private:
};

#endif // ObjectIdMaterial_h__
