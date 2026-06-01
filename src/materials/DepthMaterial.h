#pragma once

#include "MaterialBase.h"

// Override-материал для depth-pass (shadow map). Пишет только глубину из точки
// зрения света. Использует базовый Bind: model/view/projection, где в shadow-pass
// view/projection подменяются на light-матрицы через RenderContext (вариант 1).
class DepthMaterial: public MaterialBase
{
public:
	DepthMaterial(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
	~DepthMaterial() override = default;
};
