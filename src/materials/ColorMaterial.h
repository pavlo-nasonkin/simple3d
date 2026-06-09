#pragma once
#include "MaterialBase.h"
#include <memory>

class ColorMaterial: public MaterialBase
{
public:
    ColorMaterial(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
	~ColorMaterial() override = default;
	void Bind(const RenderContext& ctx, const Pivot3D* node = nullptr) override;
};