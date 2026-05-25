#include "Scene3D.h"
#include <GL/glew.h>
#include "camera/Camera.h"
#include "models/BoxModel.h"
#include "Shader.h"
#include "materials/ShaderFactory.h"
#include <memory>
#include "materials/ObjectIdMaterial.h"

Scene3D::Scene3D() :
    _lightPosition({ 6.2f, 6.0f, 6.0 }),
    _lightAmbient({ 0.2f, 0.2f, 0.2f }),
    _lightDiffuse({ 0.5f, 0.5f, 0.5f }),
    _lightSpecular({ 1.0f, 1.0f, 1.0f })
{
    std::shared_ptr<Shader> colorShade = ShaderFactory::getShader("../assets/shaders/light_source_shader.vs", "../assets/shaders/color.fs");
    _colorMaterial = std::make_shared<ObjectIdMaterial>(colorShade);
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
    std::shared_ptr<Shader> lampShader = ShaderFactory::getShader("../assets/shaders/light_source_shader.vs", "../assets/shaders/light_source_shader.fs");
    _lightSourceShader = std::make_shared<Material3D>(lampShader);
}

void Scene3D::update()
{
	//TODO update animations etc
}

void Scene3D::Render(const RenderContext& ctx, MaterialBase* material /*= nullptr*/)
{
	prepareRender();

    auto curLightMat = material != nullptr ? material : _lightSourceShader.get();
//    _lamp->render(curLightMat);

	Pivot3D::Render(ctx, material);
//	Pivot3D::render(&*_colorMaterial);

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
