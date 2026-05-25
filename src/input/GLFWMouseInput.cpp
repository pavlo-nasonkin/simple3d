#include "GLFWMouseInput.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Engine.h"


GLFWMouseInput::GLFWMouseInput(GLFWwindow* window) : _window(window)
{
	glfwSetCursorPosCallback(_window, [](GLFWwindow* window, double xPos, double yPos) {
		Engine::GetInstance().GetMouseInput()->OnMouseMove(xPos, yPos);
	});
	glfwSetMouseButtonCallback(_window, [](GLFWwindow* window, int button, int action, int mods) {
		Engine::GetInstance().GetMouseInput()->OnMouseButton(button, action);
	});
	glfwSetScrollCallback(_window, [](GLFWwindow* window, double xOffset, double yOffset) {
		Engine::GetInstance().GetMouseInput()->OnMouseScroll(xOffset, yOffset);
	});
}

GLFWMouseInput::~GLFWMouseInput()
{
	glfwSetCursorPosCallback(_window, nullptr);
	glfwSetMouseButtonCallback(_window, nullptr);
	glfwSetScrollCallback(_window, nullptr);
}