#include "Scene3D.h"
#include <GL/glew.h>
#include "Device3D.h"
#include <glm/gtc/type_ptr.hpp>
#include "camera/Camera.h"
#include "BoxModel.h"
#include "Shader.h"
#include "materials/ShaderFactory.h"
#include <memory>
#include "materials/ObjectIdMaterial.h"

#include <algorithm>


const materials_list& Scene3D::getMaterials() const
{
    return _materials;
}

void Scene3D::addMaterial(std::shared_ptr<MaterialBase> material)
{
    if (std::find(_materials.begin(), _materials.end(), material) == _materials.end()){
        _materials.push_back(material);
        dispatchEvent(MaterialBase::MATERIAL_UPDATE_EVENT);
    }
}

Camera* Scene3D::getCamera() const
{
    return _camera;
}

Scene3D::Scene3D(Camera* camera)
    :Pivot3D(), _camera(camera),
      _lightPosition({ 6.2f, 6.0f, 6.0 }),
      _lightAmbient({ 0.2f, 0.2f, 0.2f }),
      _lightDiffuse({ 0.5f, 0.5f, 0.5f }),
      _lightSpecular({ 1.0f, 1.0f, 1.0f })
{
	_camera->Position.z = 3;


    std::shared_ptr<Shader> colorShade = ShaderFactory::getShader("../assets/shaders/light_source_shader.vs", "../assets/shaders/color.fs");
    _colorMaterial = std::make_shared<ObjectIdMaterial>(ObjectIdMaterial(colorShade));
}

Scene3D::~Scene3D()
{

}

void Scene3D::initLightView()
{
    _lamp = std::make_shared<BoxModel>(BoxModel());
    _lamp->init();
	_lamp->setScale(0.1f, 0.1f, 0.1f);
	_lamp->setPosition(_lightPosition.x, _lightPosition.y, _lightPosition.z);
    std::shared_ptr<Shader> lampShader = ShaderFactory::getShader("../assets/shaders/light_source_shader.vs", "../assets/shaders/light_source_shader.fs");
    _lightSourceShader = std::make_shared<Material3D>(Material3D(lampShader));
}

void Scene3D::update()
{
	//TODO update animations etc
}

void Scene3D::render(std::shared_ptr<MaterialBase> shader /*= nullptr*/)
{
	prepareRender();
	Device3D::camera = _camera;
	Device3D::view = glm::value_ptr(_camera->GetViewMatrix());
	Device3D::projection = _camera->getProjectionMatrix();

    auto curLightMat = shader != nullptr ? shader : _lightSourceShader;
//    _lamp->render(curLightMat);

	Pivot3D::render(shader);
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

void Scene3D::init()
{
    initLightView();
}
