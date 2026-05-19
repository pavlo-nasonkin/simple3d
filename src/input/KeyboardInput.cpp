#include "KeyboardInput.h"
#include "events/IKeyboardListener.h"
#include <algorithm>

std::vector<bool> KeyboardInput::_keys = std::vector<bool>(1024, false);
std::vector<IKeyboardListener*> KeyboardInput::_listeners;

KeyboardInput::KeyboardInput()
{
}

KeyboardInput::~KeyboardInput()
{
}

void KeyboardInput::addListener(IKeyboardListener* listener)
{
	if (std::find(_listeners.begin(), _listeners.end(), listener) == _listeners.end()) {
		_listeners.push_back(listener);
	}
}

bool KeyboardInput::isKeyPressed(int key)
{
	return _keys[key];
}

void KeyboardInput::onKeyAction(int key, int action)
{

	for (IKeyboardListener* listener : _listeners)
	{
		listener->handleKeyInput(key, action);
	}

	if (action == ACTION_PRESS)
		_keys[key] = true;
	else if (action == ACTION_RELEASE)
		_keys[key] = false;
}
