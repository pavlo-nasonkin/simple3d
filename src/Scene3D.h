#pragma once

#include "Pivot3D.h"
#include <glm/glm.hpp>
#include <memory>
#include "object_selector/ObjectSelector.h"
#include "materials/ObjectIdMaterial.h"
#include "materials/MaterialBase.h"
#include "models/BoxModel.h"

class Camera;
class Shader;
class Material3D;

class BoxModel;

class Scene3D: public Pivot3D
{
	glm::vec3 _lightPosition;
    std::shared_ptr<BoxModel> _lamp;
    std::shared_ptr<MaterialBase> _lightSourceShader = nullptr;
	glm::vec3 _lightAmbient;
	glm::vec3 _lightDiffuse;
	glm::vec3 _lightSpecular;
	std::shared_ptr<ObjectIdMaterial> _colorMaterial;

public:
	Scene3D();
	~Scene3D() override;

	void update();
    void Render(const RenderContext& ctx, MaterialBase* material = nullptr) override;
	void postRender();
	void prepareRender();
    void Init() override;
	//light params
	const glm::vec3* getLightAmbient() const { return &_lightAmbient; }
	void setLightAmbient(glm::vec3 val) { _lightAmbient = val; }
	const glm::vec3* getLightDiffuse() const { return &_lightDiffuse; }
	void setLightDiffuse(glm::vec3 val) { _lightDiffuse = val; }
	const glm::vec3* getLightSpecular() const { return &_lightSpecular; }
	void setLightSpecular(glm::vec3 val) { _lightSpecular = val; }
	const glm::vec3* getLightPosition() const { return &_lightPosition; }
	void setLightPosition(glm::vec3 val) { _lightPosition = val; }

private:
    void initLightView();
};