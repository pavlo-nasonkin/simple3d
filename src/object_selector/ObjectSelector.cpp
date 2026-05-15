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



ObjectSelector::ObjectSelector()
    :_selectedObject(nullptr)
{
	MouseInput::addListener(this, MouseInput::MOUSE_BUTTON);

    std::shared_ptr<Shader> colorShade = ShaderFactory::getShader("../assets/shaders/light_source_shader.vs", "../assets/shaders/color.fs");
    _colorMaterial = std::make_shared<ObjectIdMaterial>(ObjectIdMaterial(colorShade));
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

std::shared_ptr<Pivot3D> ObjectSelector::getSelectedObject()
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
	//scene render
    Device3D::scene3D->render(_colorMaterial);
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
	_selectedObject = Device3D::scene3D->getChildById(selectedObjectId);
	fireChange(_selectedObject);

}

void ObjectSelector::fireChange(std::shared_ptr<Pivot3D> selectedObject)
{
	for (IObjectSelectorListener* listener : _objectSelectListeners)
	{
		listener->handleSelectedObject(selectedObject);
	}
}
