#pragma once

#include "input/MouseInput.h"

struct GLFWwindow;

class GLFWMouseInput: public MouseInput
{
	GLFWwindow* _window;

public:
	explicit GLFWMouseInput(GLFWwindow* window);
	~GLFWMouseInput();
};