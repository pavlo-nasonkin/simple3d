#include "MouseInput.h"
#include "events/IMouseListener.h"
#include <algorithm>

const std::string MouseInput::MOUSE_MOVE = "mouseMove";
const std::string MouseInput::MOUSE_BUTTON = "mouseButton";
const std::string MouseInput::MOUSE_SCROLL = "mouseScroll";

MouseInput::MouseInput()
{
}

void MouseInput::AddListener(IMouseListener* listener, const std::string& type)
{
	std::vector<IMouseListener*>* container = GetContainerByType(type);
	if (!container) {
		return;
	}

	if (std::find(container->begin(), container->end(), listener) == container->end())
	{
		container->push_back(listener);
	}
}

void MouseInput::RemoveListener(IMouseListener* listener, const std::string& type) {
	std::vector<IMouseListener*>* container = GetContainerByType(type);
	if (!container) {
		return;
	}
	std::erase(*container, listener);
}

void MouseInput::OnMouseMove(double xPos, double yPos)
{
	_mouseX = xPos;
	_mouseY = yPos;
	if (_mouseCaptured) {
		return;
	}
	auto listCopy = _cursorPosListeners;
	for (IMouseListener* listener : listCopy)
	{
		listener->HandleMouseMove(xPos, yPos);
	}
}

bool MouseInput::IsButtonPressed(int button) const
{
	return button >= 0 && button < kMaxButtons && _buttons[button];
}

void MouseInput::OnMouseButton(int button, int action)
{
	// Состояние кнопки трекаем всегда (даже под захватом UI), чтобы отпускание
	// не «залипало», когда курсор ушёл на панель.
	if (button >= 0 && button < kMaxButtons) {
		_buttons[button] = (action != 0); // GLFW_PRESS=1 / GLFW_RELEASE=0
	}

	if (_mouseCaptured) {
		return;
	}
	auto listCopy = _mouseButtonListeners;
	for (IMouseListener* listener : listCopy)
	{
		listener->HandleMouseButton(button, action);
	}
}

double MouseInput::ConsumeScrollY()
{
	const double value = _scrollAccumY;
	_scrollAccumY = 0.0;
	return value;
}

void MouseInput::OnMouseScroll(double xOffset, double yOffset)
{
	if (_mouseCaptured) {
		return;
	}
	_scrollAccumY += yOffset; // для поллинга из Behaviour (dolly камеры)
	auto listCopy = _scrollListeners;
	for (IMouseListener* listener : listCopy)
	{
		listener->HandleMouseScroll(xOffset, yOffset);
	}
}

std::vector<IMouseListener*>* MouseInput::GetContainerByType(const std::string &type) {
	if (type == MOUSE_MOVE) {
		return &_cursorPosListeners;
	}
	if (type == MOUSE_BUTTON) {
		return &_mouseButtonListeners;
	}
	if (type == MOUSE_SCROLL) {
		return &_scrollListeners;
	}
	return nullptr;
}
