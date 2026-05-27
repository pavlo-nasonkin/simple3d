#pragma once
#ifndef ObjectIdMaterial_h__
#define ObjectIdMaterial_h__
#include "MaterialBase.h"
class Mesh;

class ObjectIdMaterial: public MaterialBase
{
public:
    ObjectIdMaterial(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
	~ObjectIdMaterial() override = default;
	void Bind(const RenderContext& ctx, const Mesh* mesh = nullptr) override;
protected:
	
private:
};

#endif // ObjectIdMaterial_h__
