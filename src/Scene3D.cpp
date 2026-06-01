#include "Scene3D.h"
#include <GL/glew.h>
#include "camera/Camera.h"
#include "models/BoxModel.h"
#include "Shader.h"
#include "materials/ShaderFactory.h"
#include <memory>

#include "materials/Material3D.h"
#include "materials/ObjectIdMaterial.h"
#include "render/Skybox.h"
#include "resources/TextureCube.h"

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

void Scene3D::Update()
{
	//TODO update animations etc
}

void Scene3D::Render(const RenderContext& ctx, MaterialBase* material /*= nullptr*/)
{
	PrepareRender();
	Pivot3D::Render(ctx, material);
	// Skybox рисуется последним и только в обычном цветовом проходе
	// (не в id-pass'е ObjectSelector'а, где material != nullptr).
	if (_skybox && !material) {
		_skybox->Render(ctx.view, ctx.projection);
	}
    PostRender();
}

void Scene3D::SetSkybox(std::shared_ptr<TextureCube> cubemap)
{
	_skybox = std::make_shared<Skybox>(std::move(cubemap));
}

void Scene3D::PostRender()
{

}

void Scene3D::PrepareRender()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Scene3D::Init()
{
    InitLightView();
}
