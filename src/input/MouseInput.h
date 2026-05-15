#pragma once
#ifndef MouseInput_h__
#define MouseInput_h__
#include <string>
#include <vector>
class IMouseListener;

class MouseInput
{
protected:
	static double _mouseX;
	static double _mouseY;
public:
	static const std::string MOUSE_MOVE;
	static const std::string MOUSE_BUTTON;
	static const std::string MOUSE_SCROLL;
	static std::vector<IMouseListener*> _cursorPosListeners;
	static std::vector<IMouseListener*> _mouseButtonListeners;
	static std::vector<IMouseListener*> _scrollListeners;
public:
	MouseInput();
	~MouseInput();
	static void addListener(IMouseListener* listener, std::string type);
	static double mouseX() { return _mouseX; };
	static double mouseY() { return _mouseY; };

protected:
	static void onMouseMove(double xpos, double ypos);
	static void onMouseButton(int button, int action);
	static void onMouseScroll(double xoffset, double yoffset);
private:
};

#endif // MouseInput_h__