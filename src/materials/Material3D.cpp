#include "materials/Material3D.h"
#include "GLEWImporter.h"
#include "Shader.h"
#include "resources/Texture2D.h"
#include <glm/gtc/type_ptr.hpp>
#include "Scene3D.h"
#include "camera/Camera.h"
#include "Filter3d.h"
#include "utils/StringUtils.h"

Material3D::Material3D(const std::shared_ptr<Shader>& shader)
    :MaterialBase(shader)
{

}

const Material3D::FiltersList& Material3D::getFilters() const
{
    return _filters;
}

std::string Material3D::getBlendingSign(const BlendMode &blendMode)
{
    switch (blendMode)
    {
    case BlendMode::ADD:
        return " += ";

    case BlendMode::MULTIPLY:
        return " *= ";

    case BlendMode::NORMAL:
        return " = ";

    default:
        break;
    }
    return " = ";
}

void Material3D::build()
{
    if (_filters.size() > 0)
    {
        std::string fragSource = _shader->originalFragmentSource();
        std::string vertSource = _shader->originalVertexSource();
        for (auto filter : _filters)
        {
            if (filter->type() == FilterType::FRAGMENT)
            {
                StringUtils::replace(fragSource, "void main()", filter->code() + "\nvoid main()");
                size_t index = 0;
                index = fragSource.find_last_of(';');
                if (index != std::string::npos)
                {
                    const std::string& blendSign = getBlendingSign(filter->blendMode());
                    fragSource.insert(index + 1, "\n    color" + blendSign + filter->generatedUniqueName() + "();");
                }
            }
            else if (filter->type() == FilterType::VERTEX)
            {

            }
            else
            {

            }
        }
        _shader->dispose();
        _shader->setFragmentSource(fragSource);
        _shader->build();
    }
}

void Material3D::bind(const RenderContext& ctx, const Mesh* mesh/* = nullptr*/)
{
	MaterialBase::bind(ctx, mesh);

    for (auto filter : _filters)
    {
        filter->bind(_shader->Program);
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



	// Bind appropriate textures
	GLuint diffuseNr = 1;
	GLuint specularNr = 1;
	for (GLuint i = 0; i < _textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i); // Active proper texture unit before binding
										  // Retrieve texture number (the N in diffuse_textureN)
		std::stringstream ss;
		std::string number;
		std::string name = _textures[i]->type;
		if (name == "texture_diffuse")
			ss << diffuseNr++; // Transfer GLuint to stream
		else if (name == "texture_specular")
			ss << specularNr++; // Transfer GLuint to stream
		number = ss.str();
		// Now set the sampler to the correct texture unit
		glUniform1i(glGetUniformLocation(_shader->Program, (name + number).c_str()), i);
		// And finally bind the texture
		glBindTexture(GL_TEXTURE_2D, _textures[i]->id);
	}

	// Also set each mesh's shininess property to a default value (if you want you could extend this to another mesh property and possibly change this value)
	glUniform1f(glGetUniformLocation(_shader->Program, "material.shininess"), 16.0f);
}

void Material3D::unbind()
{
	MaterialBase::unbind();
	// Always good practice to set everything back to defaults once configured.
	for (GLuint i = 0; i < _textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Material3D::addFilter(std::shared_ptr<Filter3D> filter)
{
    if (filter)
    {
        _filters.push_back(filter);
    }
}

std::shared_ptr<MaterialBase> Material3D::clone() const
{
    auto result = std::make_shared<Material3D>(*this);
    result->setId(_idCounter);
    _idCounter++;
    auto shaderCopy = std::make_shared<Shader>(*_shader);
    shaderCopy->Program = 0;
    result->setShader(shaderCopy);
    return result;
}
