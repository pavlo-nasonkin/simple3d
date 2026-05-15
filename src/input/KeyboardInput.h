#pragma once
#ifndef KeyboardInput_h__
#define KeyboardInput_h__
#include <vector>

class IKeyboardListener;

class KeyboardInput
{
public:
	static const int ACTION_PRESS = 1;
	static const int ACTION_RELEASE = 0;
private:
	static bool _keys[1024];
	static std::vector<IKeyboardListener*> _listeners;
public:
	KeyboardInput();
	~KeyboardInput();
	static void addListener(IKeyboardListener* listener);
	static bool isKeyPressed(int key);
protected:
	static void onKeyAction(int key, int action);
	
private:
};

#endif // KeyboardInput_h__