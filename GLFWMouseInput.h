#pragma once
#ifndef GLFWMouseInput_h__
#define GLFWMouseInput_h__
#include <vector>
#include "input/MouseInput.h"

struct GLFWwindow;

class GLFWMouseInput: public MouseInput
{
private:
	GLFWwindow* _window;

public:
	GLFWMouseInput(GLFWwindow* window);
	~GLFWMouseInput();
	
	//TODO add remove listener
	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	
protected:
	
private:
};

#endif // !GLFWMouseInput_h__