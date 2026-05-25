#pragma once
#include "MaterialBase.h"
#include <memory>

class ColorMaterial: public MaterialBase
{
public:
    ColorMaterial(const std::shared_ptr<Shader>& shader);
	~ColorMaterial() override = default;
	void bind(const RenderContext& ctx, const Mesh* mesh = nullptr) override;
};