#include "materials/Material3D.h"
#include "GLEWImporter.h"
#include "Shader.h"
#include "resources/Texture2D.h"
#include <glm/gtc/type_ptr.hpp>
#include "Scene3D.h"
#include "camera/Camera.h"
#include "filters/Filter3d.h"
#include "utils/StringUtils.h"

Material3D::Material3D(const std::shared_ptr<Shader>& shader)
    :MaterialBase(shader)
{

}

const Material3D::FiltersList& Material3D::GetFilters() const
{
    return _filters;
}

std::string Material3D::getBlendingSign(const Filter3D::BlendMode &blendMode)
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

void Material3D::Build()
{
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


    if (_filters.size() > 0)
    {
        std::string fragSource = _shader->originalFragmentSource();
        std::string vertSource = _shader->originalVertexSource();
        for (const auto& filter : _filters)
        {
            if (filter->GetType() == Filter3D::FilterType::FRAGMENT)
            {
            	// декларации фильтра идут в один общий маркер
            	StringUtils::replace(fragSource, "// __FRAGMENT_DECLS__",
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
            	auto pos = fragSource.find(marker);
            	if (pos != std::string::npos) {
            		fragSource.replace(pos, marker.size(), injection);
            	}
            }
            else if (filter->GetType() == Filter3D::FilterType::VERTEX)
            {
            	// TODO: VERTEX filters not implemented yet
            }
            else
            {

            }
        }

		std::cout << "Built shader with filters. Vertex shader:\n" << vertSource << "\nFragment shader:\n" << fragSource << std::endl;

        _shader->dispose();
        _shader->setFragmentSource(fragSource);
        _shader->build();
    }
}

void Material3D::Bind(const RenderContext& ctx, const Mesh* mesh/* = nullptr*/)
{
	MaterialBase::Bind(ctx, mesh);

	GLuint nextUnit = 0;
    for (const auto& filter : _filters)
    {
        filter->Bind(_shader->Program, nextUnit);
    	nextUnit += filter->GetUniformsCount();
    }
	
	GLint modelLoc = glGetUniformLocation(_shader->Program, "model");
	GLint viewLoc = glGetUniformLocation(_shader->Program, "view");
	GLint projectionLoc = glGetUniformLocation(_shader->Program, "projection");
	GLint viewPosLoc = glGetUniformLocation(_shader->Program, "viewPos");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ctx.model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(ctx.view));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(ctx.projection));
	glUniform3f(viewPosLoc, ctx.camera->Position.x, ctx.camera->Position.y, ctx.camera->Position.z);


	GLint lightPositionLoc = glGetUniformLocation(_shader->Program, "light.position");
	GLint lightAmbientLoc = glGetUniformLocation(_shader->Program, "light.ambient");
	GLint lightDiffuseLoc = glGetUniformLocation(_shader->Program, "light.diffuse");
	GLint lightSpecularLoc = glGetUniformLocation(_shader->Program, "light.specular");
	const glm::vec3* lightAmbient = ctx.scene3D->getLightAmbient();
	const glm::vec3* lightDiffuce = ctx.scene3D->getLightDiffuse();
	const glm::vec3* lightSpecular = ctx.scene3D->getLightSpecular();
	const glm::vec3* lightPos = ctx.scene3D->getLightPosition();

	glUniform3f(lightAmbientLoc, lightAmbient->x, lightAmbient->y, lightAmbient->z);
	glUniform3f(lightDiffuseLoc, lightDiffuce->x, lightDiffuce->y, lightDiffuce->z); // Let's darken the light a bit to fit the scene
	glUniform3f(lightSpecularLoc, lightSpecular->x, lightSpecular->y, lightSpecular->z);
	glUniform3f(lightPositionLoc, lightPos->x, lightPos->y, lightPos->z);

	GLint baseColorLoc = glGetUniformLocation(_shader->Program, "uBaseColor");
	glUniform4f(baseColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

	// Also set each mesh's shininess property to a default value (if you want you could extend this to another mesh property and possibly change this value)
	glUniform1f(glGetUniformLocation(_shader->Program, "material.shininess"), 16.0f);
}

void Material3D::Unbind()
{
	MaterialBase::Unbind();
	GLuint unit = 0;
	for (auto& filter : _filters) {
		for (unsigned int i = 0; i < filter->GetUniformsCount(); ++i) {
			glActiveTexture(GL_TEXTURE0 + unit + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		unit += filter->GetUniformsCount();
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

std::shared_ptr<MaterialBase> Material3D::Clone() const
{
    auto result = std::make_shared<Material3D>(*this);
    result->setId(_idCounter);
    _idCounter++;
    auto shaderCopy = std::make_shared<Shader>(*_shader);
    shaderCopy->Program = 0;
    result->setShader(shaderCopy);
    return result;
}
