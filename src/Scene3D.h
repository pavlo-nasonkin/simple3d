#pragma once

#include "Pivot3D.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
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
class ShadowMap;
class DepthMaterial;

class SceneEnvironment {
	std::shared_ptr<TextureCube> _irradiance;        // diffuse IBL
	std::shared_ptr<TextureCube> _prefilteredSpec;   // specular IBL
	std::shared_ptr<Texture2D>   _brdfLUT;
public:
	void Set(std::shared_ptr<TextureCube> irradiance,
	         std::shared_ptr<TextureCube> prefilteredSpec,
	         std::shared_ptr<Texture2D> brdfLUT) {
		_irradiance = std::move(irradiance);
		_prefilteredSpec = std::move(prefilteredSpec);
		_brdfLUT = std::move(brdfLUT);
	}
	bool IsValid() const { return _irradiance && _prefilteredSpec && _brdfLUT; }
	const TextureCube* Irradiance() const { return _irradiance.get(); }
	const TextureCube* PrefilteredSpec() const { return _prefilteredSpec.get(); }
	const Texture2D*   BrdfLUT() const { return _brdfLUT.get(); }
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

	std::shared_ptr<ShadowMap> _shadowMap;
	std::shared_ptr<DepthMaterial> _depthMaterial;
	glm::vec3 _shadowCenter { 0.0f, 0.0f, 0.0f };
	float _shadowRadius = 20.0f;
	float _shadowStrength = 0.7f; // насколько тень притеняет ambient (0 = только direct, 1 = ambient в тени гаснет)

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
	void SetDirLightDirection(const glm::vec3& v) { _dirLightDirection = v; }
	void SetDirLightColor(const glm::vec3& v) { _dirLightColor = v; }
	void SetDirLightIntensity(float v) { _dirLightIntensity = v; }

	// Включает отрисовку фона из cubemap'а (создаёт Skybox внутри). Требует GL-контекста.
	void SetSkybox(std::shared_ptr<TextureCube> cubemap);

	// Загружает HDR, печёт IBL, ставит skybox и запоминает путь (для сериализации сцены).
	void SetEnvironmentFromHdr(const std::string& path);
	const std::string& GetHdrPath() const { return _hdrPath; }

	// IBL-окружение (irradiance + prefiltered specular + BRDF LUT) для PBRLightingModel.
	void SetEnvironment(std::shared_ptr<TextureCube> irradiance,
	                    std::shared_ptr<TextureCube> prefilteredSpec,
	                    std::shared_ptr<Texture2D> brdfLUT) {
		_environment.Set(std::move(irradiance), std::move(prefilteredSpec), std::move(brdfLUT));
	}
	const SceneEnvironment& GetEnvironment() const { return _environment; }

	// Включает directional shadow mapping (создаёт ShadowMap + DepthMaterial). Требует GL-контекста.
	void EnableShadows(int mapSize = 2048, float radius = 20.0f);
	void SetShadowArea(const glm::vec3& center, float radius) { _shadowCenter = center; _shadowRadius = radius; }
	float GetShadowStrength() const { return _shadowStrength; }
	void SetShadowStrength(float value) { _shadowStrength = value; }
	float GetShadowRadius() const { return _shadowRadius; }
	const glm::vec3& GetShadowCenter() const { return _shadowCenter; }

private:
	std::string _hdrPath;
    void InitLightView();
	void InitEnvironment();
};