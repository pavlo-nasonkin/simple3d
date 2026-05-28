#include "materials/Material3D.h"

#include <iostream>

#include "GLEWImporter.h"
#include "Shader.h"
#include "resources/Texture2D.h"
#include <glm/gtc/type_ptr.hpp>
#include "Scene3D.h"
#include "ShaderFactory.h"
#include "camera/Camera.h"
#include "filters/Filter3d.h"
#include "lighting/ILightingModel.h"
#include "lighting/PhongLightingModel.h"
#include "utils/StringUtils.h"

Material3D::Material3D(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
    :MaterialBase(vertexShaderPath, fragmentShaderPath), _lighting(std::make_unique<PhongLightingModel>())
{

}

const Material3D::FiltersList& Material3D::GetFilters() const
{
    return _filters;
}

void Material3D::Build() {
	MaterialBase::Build();
	if (_lighting) {
		_lighting->OnProgramBuild(_shader->GetProgram());
	}
}

std::string Material3D::getBlendingSign(const Filter3D::BlendMode &blendMode) const
{
    switch (blendMode)
    {
    case Filter3D::BlendMode::ADD:
        return " += ";

    case Filter3D::BlendMode::MULTIPLY:
        return " *= ";

    case Filter3D::BlendMode::NORMAL:
        return " = ";

    default:
        break;
    }
    return " = ";
}

const ShaderFactory::CompiledShader& Material3D::BuildFragmentShader() const {
	std::string fragSource = ShaderFactory::GetShaderSource(_fragmentShaderPath);
	InjectFilters(fragSource);

	if (_lighting) {
		StringUtils::replace(fragSource, "// __LIGHTING_DECLS__",
							 _lighting->GetDeclarations() + "\n");

		StringUtils::replace(fragSource, "// __APPLY_LIGHTING__",
							 _lighting->GetLightingCode() + "\n");
	}


	return ShaderFactory::GetCompiledShaderFromSource(GL_FRAGMENT_SHADER, fragSource);
}

void Material3D::InjectFilters(std::string &fragmentShaderSource) const {

	static const std::unordered_map<Filter3D::FilterSlot, std::string> kSlotMarkers = {
		{ Filter3D::FilterSlot::BaseColor, "// __APPLY_BASE_COLOR_FILTERS__" },
		{ Filter3D::FilterSlot::Specular,  "// __APPLY_SPECULAR_FILTERS__"  },
		{ Filter3D::FilterSlot::Normal,    "// __APPLY_NORMAL_FILTERS__"    },
		{ Filter3D::FilterSlot::Emissive,  "// __APPLY_EMISSIVE_FILTERS__"  },
		{ Filter3D::FilterSlot::Overlay,   "// __APPLY_OVERLAY_FILTERS__"   },
	};
	static const std::unordered_map<Filter3D::FilterSlot, std::string> kSlotTarget = {
		{ Filter3D::FilterSlot::BaseColor, "BASE_COLOR" },
		{ Filter3D::FilterSlot::Specular,  "SPEC_STRENGTH" },
		{ Filter3D::FilterSlot::Normal,    "N" },
		{ Filter3D::FilterSlot::Emissive,  "EMISSIVE" },
		{ Filter3D::FilterSlot::Overlay,   "color" },
	};

	for (const auto& filter : _filters)
	{
		if (filter->GetType() != Filter3D::FilterType::FRAGMENT)
		{
			continue; // пока поддерживаем только фрагментные фильтры
		}

		// декларации фильтра идут в один общий маркер
		StringUtils::replace(fragmentShaderSource, "// __FRAGMENT_DECLS__",
							 filter->GetCode() + "\n// __FRAGMENT_DECLS__");

		const auto& marker = kSlotMarkers.at(filter->GetSlot());
		const auto& target = kSlotTarget.at(filter->GetSlot());
		const auto& blend  = getBlendingSign(filter->GetBlendMode());

		const bool targetIsVec3 = (filter->GetSlot() == Filter3D::FilterSlot::BaseColor
				|| filter->GetSlot() == Filter3D::FilterSlot::Specular
				|| filter->GetSlot() == Filter3D::FilterSlot::Normal
				|| filter->GetSlot() == Filter3D::FilterSlot::Emissive);

		const bool returnsVec4  = (filter->GetResultType() == Filter3D::ResultType::VEC4);
		std::string rhs = filter->GetGeneratedUniqueName() + "()";
		if (targetIsVec3 && returnsVec4) {
			rhs += ".rgb";
		}

		std::string injection = target + blend + rhs + ";\n    " + marker;
		auto pos = fragmentShaderSource.find(marker);
		if (pos != std::string::npos) {
			fragmentShaderSource.replace(pos, marker.size(), injection);
		}
	}
}

void Material3D::Bind(const RenderContext& ctx, const Mesh* mesh/* = nullptr*/)
{
	MaterialBase::Bind(ctx, mesh);

	auto program = _shader->GetProgram();
	GLuint nextUnit = 0;
    for (const auto& filter : _filters)
    {
        filter->Bind(program, nextUnit);
    	nextUnit += filter->GetUniformsCount();
    }

	if (_lighting) {
		_lighting->Bind(nextUnit, ctx);
		nextUnit += _lighting->GetTextureUnitCount();
	}

	GLint baseColorLoc = _uniformCache.GetUniformLocation("uBaseColor");
	glUniform4f(baseColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
}

void Material3D::Unbind()
{
	MaterialBase::Unbind();
	GLuint nextUnit = 0;
	auto program = _shader->GetProgram();
	for (const auto& filter : _filters) {
		filter->Unbind(program, nextUnit);
		nextUnit += filter->GetUniformsCount();
	}

	if (_lighting) {
		_lighting->Unbind(nextUnit);
		nextUnit += _lighting->GetTextureUnitCount();
	}
}

void Material3D::AddFilter(const std::shared_ptr<Filter3D>& filter)
{
    if (filter)
    {
    	filter->SetId(_filters.size());
    	filter->SetNextUniformId(_nextUniformId);
    	filter->Init();
    	_nextUniformId += filter->GetUniformsCount();
        _filters.push_back(filter);
    }
}