#include "Scene3D.h"
#include <GL/glew.h>
#include "camera/Camera.h"
#include "models/BoxModel.h"
#include "Shader.h"
#include "materials/ShaderFactory.h"
#include <memory>

#include "materials/Material3D.h"
#include "materials/ObjectIdMaterial.h"

Scene3D::Scene3D() :
    _lightPosition({ 6.2f, 6.0f, 6.0 }),
    _lightAmbient({ 0.2f, 0.2f, 0.2f }),
    _lightDiffuse({ 0.5f, 0.5f, 0.5f }),
    _lightSpecular({ 1.0f, 1.0f, 1.0f })
{

}

Scene3D::~Scene3D()
{

}

void Scene3D::initLightView()
{
    _lamp = std::make_shared<BoxModel>();
    _lamp->Init();
	_lamp->SetScale(0.1f, 0.1f, 0.1f);
	_lamp->SetPosition(_lightPosition.x, _lightPosition.y, _lightPosition.z);
    // AddChild(_lamp);
}

void Scene3D::update()
{
	//TODO update animations etc
}

void Scene3D::Render(const RenderContext& ctx, MaterialBase* material /*= nullptr*/)
{
	prepareRender();
	Pivot3D::Render(ctx, material);
    postRender();
}

void Scene3D::postRender()
{

}

void Scene3D::prepareRender()
{
	glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Scene3D::Init()
{
    initLightView();
}
