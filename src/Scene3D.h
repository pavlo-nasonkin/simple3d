#pragma once

#include "Pivot3D.h"
#include <glm/glm.hpp>
#include <memory>
#include "materials/MaterialBase.h"
#include "models/BoxModel.h"
#include "resources/Texture2D.h"
#include "resources/TextureCube.h"

class Camera;
class Shader;
class Material3D;

class BoxModel;
class Skybox;
class TextureCube;

class SceneEnvironment {
	std::shared_ptr<TextureCube> irradiance;        // diffuse IBL
	std::shared_ptr<TextureCube> prefilteredSpec;   // specular IBL
	std::shared_ptr<Texture2D>   brdfLUT;
};

class Scene3D: public Pivot3D
{
	glm::vec3 _lightPosition;
    std::shared_ptr<BoxModel> _lamp;
	glm::vec3 _lightAmbient;
	glm::vec3 _lightDiffuse;
	glm::vec3 _lightSpecular;

	glm::vec3 _dirLightDirection { -0.5f, -1.0f, -0.3f }; // куда светит (от солнца к сцене)
	glm::vec3 _dirLightColor     {  1.0f,  0.96f, 0.90f }; // тон солнца, нормирован
	float     _dirLightIntensity =  3.0f;                  // компенсация деления на π

	SceneEnvironment _environment;
	std::shared_ptr<Skybox> _skybox;

public:
	Scene3D();
	~Scene3D() override = default;

	void Update();
    void Render(const RenderContext& ctx, MaterialBase* material = nullptr) override;
	void PostRender();
	void PrepareRender();
    void Init() override;
	//light params
	const glm::vec3* GetLightAmbient() const { return &_lightAmbient; }
	void SetLightAmbient(glm::vec3 val) { _lightAmbient = val; }
	const glm::vec3* GetLightDiffuse() const { return &_lightDiffuse; }
	void SetLightDiffuse(glm::vec3 val) { _lightDiffuse = val; }
	const glm::vec3* GetLightSpecular() const { return &_lightSpecular; }
	void SetLightSpecular(glm::vec3 val) { _lightSpecular = val; }
	const glm::vec3* GetLightPosition() const { return &_lightPosition; }
	void SetLightPosition(glm::vec3 val) { _lightPosition = val; }

	const glm::vec3* GetDirLightDirection() const { return &_dirLightDirection; }
	const glm::vec3* GetDirLightColor() const { return &_dirLightColor; }
	float GetDirLightIntensity() const { return _dirLightIntensity; }

	// Включает отрисовку фона из cubemap'а (создаёт Skybox внутри). Требует GL-контекста.
	void SetSkybox(std::shared_ptr<TextureCube> cubemap);

private:
    void InitLightView();
	void InitEnvironment();
};