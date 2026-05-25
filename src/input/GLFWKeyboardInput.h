#pragma once

#include "input/KeyboardInput.h"
struct GLFWwindow;

class GLFWKeyboardInput: public KeyboardInput{

	GLFWwindow* _window;

public:
	explicit GLFWKeyboardInput(GLFWwindow* window);
	~GLFWKeyboardInput();
};