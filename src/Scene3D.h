#pragma once
#ifndef Scene3D_h__
#define Scene3D_h__

#include "Pivot3D.h"
#include <glm/glm.hpp>
#include <memory>
#include "object_selector/ObjectSelector.h"
#include "materials/ObjectIdMaterial.h"
#include "materials/MaterialBase.h"
#include "BoxModel.h"

class Camera;
class Shader;
class Material3D;

class BoxModel;

typedef std::vector<std::shared_ptr<MaterialBase>> materials_list;

class Scene3D: public Pivot3D
{
private:
	Camera* _camera;
	glm::vec3 _lightPosition;
    std::shared_ptr<BoxModel> _lamp;
    std::shared_ptr<MaterialBase> _lightSourceShader = nullptr;
	glm::vec3 _lightAmbient;
	glm::vec3 _lightDiffuse;
	glm::vec3 _lightSpecular;
    materials_list _materials;
	std::shared_ptr<ObjectIdMaterial> _colorMaterial;

public:
	Scene3D(Camera* camera);
	~Scene3D();

	void update();
    void render(std::shared_ptr<MaterialBase> shader = nullptr) override;
	void postRender();
	void prepareRender();
    void init() override;
    const materials_list& getMaterials() const;
    void addMaterial(std::shared_ptr<MaterialBase> material);
	//light params
	const glm::vec3* getLightAmbient() const { return &_lightAmbient; }
	void setLightAmbient(glm::vec3 val) { _lightAmbient = val; }
	const glm::vec3* getLightDiffuse() const { return &_lightDiffuse; }
	void setLightDiffuse(glm::vec3 val) { _lightDiffuse = val; }
	const glm::vec3* getLightSpecular() const { return &_lightSpecular; }
	void setLightSpecular(glm::vec3 val) { _lightSpecular = val; }
	const glm::vec3* getLightPosition() const { return &_lightPosition; }
	void setLightPosition(glm::vec3 val) { _lightPosition = val; }
    Camera* getCamera() const;

protected:

private:
    void initLightView();
};

#endif // Scene3D_h__
