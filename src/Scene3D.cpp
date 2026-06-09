#include "Scene3D.h"
#include <GL/glew.h>
#include "camera/Camera.h"
#include "models/BoxModel.h"
#include "Shader.h"
#include "materials/ShaderFactory.h"
#include <memory>

#include "materials/Material3D.h"
#include "materials/ObjectIdMaterial.h"
#include "materials/DepthMaterial.h"
#include "render/Skybox.h"
#include "render/ShadowMap.h"
#include "render/IBLBaker.h"
#include "resources/TextureCube.h"
#include "resources/HDRLoader.h"
#include "utils/AssetPaths.h"
#include "behaviours/LightComponent.h"

#include <functional>

Scene3D::Scene3D() :
    _lightPosition({ 5.2f, 5.0f, 5.0 }),
    _lightAmbient({ 0.03f, 0.03f, 0.03f }),
    _lightDiffuse({ 1.0f, 0.96f, 0.90f }),
    _lightSpecular({ 1.0f, 1.0f, 1.0f })
{

}

void Scene3D::InitLightView()
{
    _lamp = std::make_shared<BoxModel>();
    _lamp->Init();
	_lamp->SetScale(0.1f, 0.1f, 0.1f);
	_lamp->SetPosition(_lightPosition.x, _lightPosition.y, _lightPosition.z);
    // AddChild(_lamp);
}

void Scene3D::InitEnvironment()
{

}

void Scene3D::Update(float deltaTime)
{
	// Тик логики: компоненты (Behaviour) всего поддерева сцены.
	// TODO: сюда же обновление анимаций и пр.
	UpdateSubtree(deltaTime);
}

DirectionalLightComponent* Scene3D::FindActiveDirectionalLight()
{
	DirectionalLightComponent* found = nullptr;
	std::function<void(Pivot3D&)> visit = [&](Pivot3D& node) {
		if (found) return;
		for (const auto& behaviour : node.GetBehaviours()) {
			if (auto* light = dynamic_cast<DirectionalLightComponent*>(behaviour.get())) {
				if (light->IsEnabled()) { found = light; return; }
			}
		}
		for (const auto& child : node.Children()) {
			if (found) return;
			visit(*child);
		}
	};
	for (const auto& child : Children()) {
		if (found) break;
		visit(*child);
	}
	return found;
}

glm::vec3 Scene3D::GetEffectiveDirLightDirection() const
{
	return _activeDirLight ? _activeDirLight->GetWorldDirection() : _dirLightDirection;
}

glm::vec3 Scene3D::GetEffectiveDirLightColor() const
{
	return _activeDirLight ? _activeDirLight->GetColor() : _dirLightColor;
}

float Scene3D::GetEffectiveDirLightIntensity() const
{
	return _activeDirLight ? _activeDirLight->GetIntensity() : _dirLightIntensity;
}

glm::vec3 Scene3D::GetEffectiveAmbient() const
{
	return _activeDirLight ? _activeDirLight->GetAmbient() : _lightAmbient;
}

void Scene3D::RefreshActiveLights()
{
	// Активный направленный свет ищем заново каждый кадр (без висячих указателей
	// после загрузки сцены); далее PBR и shadow-pass читают «эффективные» геттеры.
	_activeDirLight = FindActiveDirectionalLight();
}

void Scene3D::EnableShadows(int mapSize, float radius)
{
	_shadowMap = std::make_shared<ShadowMap>(mapSize);
	_depthMaterial = std::make_shared<DepthMaterial>(AssetPaths::Resolve("shaders/depth.vsh"),
	                                                 AssetPaths::Resolve("shaders/depth.fsh"));
	_depthMaterial->Build();
	_shadowRadius = radius;
}

void Scene3D::SetSkybox(std::shared_ptr<TextureCube> cubemap)
{
	_skybox = std::make_shared<Skybox>(std::move(cubemap));
}

void Scene3D::SetEnvironmentFromHdr(const std::string& path)
{
	auto cubemap = HDRLoader::EquirectFileToCubemap(path);
	if (!cubemap) {
		return;
	}
	_hdrPath = path;
	SetSkybox(cubemap);
	auto irradiance  = IBLBaker::BakeIrradiance(*cubemap);
	auto prefiltered = IBLBaker::BakePrefiltered(*cubemap);
	auto brdfLUT     = IBLBaker::BakeBRDFLUT();
	SetEnvironment(irradiance, prefiltered, brdfLUT);
}

void Scene3D::Init()
{
    InitLightView();
}
