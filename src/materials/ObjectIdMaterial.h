#pragma once
#ifndef ObjectIdMaterial_h__
#define ObjectIdMaterial_h__
#include "MaterialBase.h"
class Mesh;

class ObjectIdMaterial: public MaterialBase
{
public:
    ObjectIdMaterial(const std::shared_ptr<Shader>& shader);
	~ObjectIdMaterial() override = default;
	void bind(const RenderContext& ctx, const Mesh* mesh = nullptr) override;
protected:
	
private:
};

#endif // ObjectIdMaterial_h__
