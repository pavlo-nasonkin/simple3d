#pragma once
#ifndef I_KEYBOARD_LISTENER_H
#define I_KEYBOARD_LISTENER_H

class IKeyboardListener
{
public:
	virtual void handleKeyInput(int key, int action) = 0;
};


#endif // !I_KEYBOARD_LISTENER_H
