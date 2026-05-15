#include "MouseInput.h"
#include "events/IMouseListener.h"
#include <algorithm>

double MouseInput::_mouseX = 0.0;
double MouseInput::_mouseY = 0.0;

const std::string MouseInput::MOUSE_MOVE = "mouseMove";
const std::string MouseInput::MOUSE_BUTTON = "mouseButton";
const std::string MouseInput::MOUSE_SCROLL = "mouseScroll";

std::vector<IMouseListener*> MouseInput::_cursorPosListeners;
std::vector<IMouseListener*> MouseInput::_mouseButtonListeners;
std::vector<IMouseListener*> MouseInput::_scrollListeners;

MouseInput::MouseInput()
{
}

MouseInput::~MouseInput()
{
	_cursorPosListeners.clear();
}

void MouseInput::addListener(IMouseListener* listener, std::string type)
{
	std::vector<IMouseListener*>* container;
	if (type == MOUSE_MOVE) {
		container = &_cursorPosListeners;
	}
	else if (type == MOUSE_BUTTON) {
		container = &_mouseButtonListeners;
	}
	else {
		container = &_scrollListeners;
	}

	if (std::find(container->begin(), container->end(), listener) == container->end())
	{
		container->push_back(listener);
	}
}

void MouseInput::onMouseMove(double xpos, double ypos)
{
	for (IMouseListener* listener : _cursorPosListeners)
	{
		listener->handleMouseMove(xpos, ypos);
	}
	_mouseX = xpos;
	_mouseY = ypos;
}

void MouseInput::onMouseButton(int button, int action)
{
	for (IMouseListener* listener : _mouseButtonListeners)
	{
		listener->handleMouseButton(button, action);
	}
}

void MouseInput::onMouseScroll(double xoffset, double yoffset)
{
	for (IMouseListener* listener : _scrollListeners)
	{
		listener->handleMouseScroll(xoffset, yoffset);
	}
}
