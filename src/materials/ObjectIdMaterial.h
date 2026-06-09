#pragma once

#include "MaterialBase.h"
class Pivot3D;

class ObjectIdMaterial: public MaterialBase
{
public:
    ObjectIdMaterial(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
	~ObjectIdMaterial() override = default;
	void Bind(const RenderContext& ctx, const Pivot3D* node = nullptr) override;
};