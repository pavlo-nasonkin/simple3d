#include "GLFWMouseInput.h"
#include <GLFW/glfw3.h>
#include <iostream>



GLFWMouseInput::GLFWMouseInput(GLFWwindow* window)
	:MouseInput(), _window(window)
{
	glfwSetCursorPosCallback(_window, cursorPositionCallback);
	glfwSetMouseButtonCallback(_window, mouseButtonCallback);
	glfwSetScrollCallback(_window, scrollCallback);
}

GLFWMouseInput::~GLFWMouseInput()
{
	
	glfwSetCursorPosCallback(_window, NULL);
	glfwSetMouseButtonCallback(_window, NULL);
	glfwSetScrollCallback(_window, NULL);
}

void GLFWMouseInput::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	MouseInput::onMouseMove(xpos, ypos);
}

void GLFWMouseInput::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	MouseInput::onMouseButton(button, action);
}

void GLFWMouseInput::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	MouseInput::onMouseScroll(xoffset, yoffset);
	
}
