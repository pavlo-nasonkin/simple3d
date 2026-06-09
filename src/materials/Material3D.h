#pragma once

#include <vector>
#include "MaterialBase.h"
#include <memory>

#include "filters/Filter3d.h"

class ILightingModel;
class Shader;
class Texture2D;
class Pivot3D;
class Filter3D;

class Material3D: public MaterialBase
{
    using FiltersList = std::vector<std::shared_ptr<Filter3D>>;

public:
    explicit Material3D(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
    ~Material3D() override = default;
	void Bind(const RenderContext& ctx, const Pivot3D* node = nullptr) override;
	void Unbind() override;
    void AddFilter(const std::shared_ptr<Filter3D>& filter);

    // Сборка из готового кода (без InjectFilters). Фильтры должны быть уже добавлены
    // (для биндинга текстур); их uniform-id совпадают с кэшем за счёт детерминизма.
    void BuildCompiled(const std::string& vertexSource, const std::string& fragmentSource);

    // Версия кодогенератора шейдеров. Бамп при изменении генерации/инъекции.
    static int ShaderGenVersion() { return 1; }
    void SetLightingModel(std::unique_ptr<ILightingModel> model) { _lighting = std::move(model); }
    void SetRoughnessScale(float scale) { _roughnessScale = scale; }
    float GetRoughnessScale() const { return _roughnessScale; }
    std::string GetLightingTypeName() const; // тип текущей lighting-модели ("PBR"/"Phong"/"Unlit")
    const FiltersList& GetFilters() const;
    void Build() override;
protected:
    const ShaderFactory::CompiledShader& BuildFragmentShader() const override;
private:
    void InjectFilters(std::string& fragmentShaderSource) const;
    std::string getBlendingSign(const Filter3D::BlendMode& blendMode) const;

    FiltersList _filters;
    unsigned int _nextUniformId = 0;
    std::unique_ptr<ILightingModel> _lighting;
    float _roughnessScale = 1.0f;
};