#include "ObjectSelector.h"
#include "input/MouseInput.h"
#include <iostream>
#include <algorithm>
#include "Shader.h"
#include "materials/ShaderFactory.h"
#include "materials/ObjectIdMaterial.h"
#include "Scene3D.h"

#include <GLFW/glfw3.h>
#include "Device3D.h"
#include "events/IObjectSelectorListener.h"
#include <memory>

#include "Engine.h"
#include "camera/Camera.h"


ObjectSelector::ObjectSelector(const std::shared_ptr<Scene3D> &scene, const std::shared_ptr<Camera>& camera)
    :_selectedObject(nullptr), _scene(scene), _camera(camera)
{
	MouseInput::addListener(this, MouseInput::MOUSE_BUTTON);

    std::shared_ptr<Shader> colorShade = ShaderFactory::getShader("../assets/shaders/light_source_shader.vs", "../assets/shaders/color.fs");
    _colorMaterial = std::make_shared<ObjectIdMaterial>(colorShade);
}

ObjectSelector::~ObjectSelector()
{
	//TODO
	//MouseInput::removeListener(this, MouseInput::MOUSE_BUTTON);
	//delete _colorMaterial;
}

void ObjectSelector::addSelectListener(IObjectSelectorListener* listener)
{
	if (std::find(_objectSelectListeners.begin(), _objectSelectListeners.end(), listener) == _objectSelectListeners.end())
	{
		_objectSelectListeners.push_back(listener);
	}
}

void ObjectSelector::removeSelectListener(IObjectSelectorListener* /*listener*/)
{
	//TODO
}

void ObjectSelector::handleMouseButton(int button, int action)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		pickObject();
	}
}

const std::shared_ptr<Pivot3D>& ObjectSelector::getSelectedObject()
{
	return _selectedObject;
}

void ObjectSelector::setSelectedObject(std::shared_ptr<Pivot3D> val)
{
	_selectedObject = val;
}

void ObjectSelector::readPixel(void* pixel)
{
	glReadPixels(static_cast<GLint>(MouseInput::mouseX()),
		static_cast<GLint>(Device3D::sceenHeight - MouseInput::mouseY()),
		1, 1,
		GL_RGB, GL_UNSIGNED_BYTE,
		pixel);
}

void ObjectSelector::pickObject()
{
	//TODO set render to texture

	auto scene3D = _scene.lock();
	if (!scene3D) {
		return;
	}

	auto camera = _camera.lock();
	if (!camera) {
		return;
	}

	//scene render
	RenderContext ctx;
	ctx.camera = camera.get();
	ctx.view = ctx.camera->GetViewMatrix();
	ctx.projection = ctx.camera->getProjectionMatrix();
    scene3D->render(ctx, _colorMaterial.get());
	//get pixel
	//set render to backbuffer
	unsigned char pixel[4];
	pixel[3] = 0;

	std::cout << "mouseXY " << MouseInput::mouseX() << "  " << MouseInput::mouseY() << std::endl;
	std::cout << "screen" << Device3D::sceenWidth << "  " << Device3D::sceenHeight << std::endl;
	readPixel(pixel);
	std::cout << "R: " << (int)pixel[0] << std::endl;
	std::cout << "G: " << (int)pixel[1] << std::endl;
	std::cout << "B: " << (int)pixel[2] << std::endl;
	std::cout << std::endl;
	unsigned int selectedObjectId = (pixel[3] << 24) | (pixel[2] << 16) | (pixel[1] << 8) | (pixel[0]);
	std::cout << "ObjectId = " << selectedObjectId << std::endl;
	_selectedObject = scene3D->getChildById(selectedObjectId);
	fireChange(_selectedObject);

}

void ObjectSelector::fireChange(std::shared_ptr<Pivot3D> selectedObject)
{
	for (IObjectSelectorListener* listener : _objectSelectListeners)
	{
		listener->handleSelectedObject(selectedObject);
	}
}
