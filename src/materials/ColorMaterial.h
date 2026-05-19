#pragma once
#ifndef ColorMaterial_h__
#define ColorMaterial_h__
#include "MaterialBase.h"
#include <memory>

class ColorMaterial: public MaterialBase
{
public:
    ColorMaterial(std::shared_ptr<Shader> shader);
	~ColorMaterial() override;
	void bind(const RenderContext& ctx, const Mesh* mesh = nullptr) override;
protected:
	
private:
};

#endif // ColorMaterial_h___
