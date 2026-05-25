#pragma once
#include <vector>

class IKeyboardListener;

class KeyboardInput
{
public:
	static constexpr int ACTION_PRESS = 1;
	static constexpr int ACTION_RELEASE = 0;

	KeyboardInput();
	~KeyboardInput() = default;
	void AddListener(IKeyboardListener* listener);
	void RemoveListener(IKeyboardListener* listener);
	bool IsKeyPressed(int key) const;

	void OnKeyAction(int key, int action);

private:
	std::vector<bool> _keys = std::vector<bool>(1024, false);
	std::vector<IKeyboardListener*> _listeners;
};