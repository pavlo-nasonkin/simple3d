#pragma once

class IKeyboardListener
{
public:
	virtual ~IKeyboardListener() = default;

	virtual void HandleKeyInput(int key, int action) = 0;
};