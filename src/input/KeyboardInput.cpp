#include "KeyboardInput.h"
#include "events/IKeyboardListener.h"
#include <algorithm>

KeyboardInput::KeyboardInput()
{
}

void KeyboardInput::AddListener(IKeyboardListener* listener)
{
	if (std::find(_listeners.begin(), _listeners.end(), listener) == _listeners.end()) {
		_listeners.push_back(listener);
	}
}

void KeyboardInput::RemoveListener(IKeyboardListener *listener) {
	std::erase(_listeners, listener);
}

bool KeyboardInput::IsKeyPressed(int key) const
{
	return key >= 0 && key < static_cast<int>(_keys.size()) && _keys[key];
}

void KeyboardInput::OnKeyAction(int key, int action)
{
	if (key < 0 || key >= static_cast<int>(_keys.size())) {
		return;
	}

	auto listCopy = _listeners;
	for (IKeyboardListener* listener : listCopy)
	{
		listener->HandleKeyInput(key, action);
	}

	if (action == ACTION_PRESS)
		_keys[key] = true;
	else if (action == ACTION_RELEASE)
		_keys[key] = false;
}
