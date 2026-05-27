#pragma once
#include "MaterialBase.h"
#include <memory>

class ColorMaterial: public MaterialBase
{
public:
    ColorMaterial(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
	~ColorMaterial() override = default;
	void Bind(const RenderContext& ctx, const Mesh* mesh = nullptr) override;
};