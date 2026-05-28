#pragma once

#include "MaterialBase.h"
class Mesh;

class ObjectIdMaterial: public MaterialBase
{
public:
    ObjectIdMaterial(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
	~ObjectIdMaterial() override = default;
	void Bind(const RenderContext& ctx, const Mesh* mesh = nullptr) override;
};