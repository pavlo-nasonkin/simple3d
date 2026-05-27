#pragma once

#include <vector>
#include "MaterialBase.h"
#include <memory>

#include "filters/Filter3d.h"

class Shader;
class Texture2D;
class Mesh;
class Filter3D;

class Material3D: public MaterialBase
{
    using FiltersList = std::vector<std::shared_ptr<Filter3D>>;

public:
    explicit Material3D(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
    ~Material3D() override = default;
	void Bind(const RenderContext& ctx, const Mesh* mesh = nullptr) override;
	void Unbind() override;
    void AddFilter(const std::shared_ptr<Filter3D>& filter);
    const FiltersList& GetFilters() const;
    std::shared_ptr<MaterialBase> Clone() const override;
protected:
    const ShaderFactory::CompiledShader& BuildFragmentShader() const override;
private:
    void InjectFilters(std::string& fragmentShaderSource) const;
    std::string getBlendingSign(const Filter3D::BlendMode& blendMode) const;

    FiltersList _filters;
    unsigned int _nextUniformId = 0;
};