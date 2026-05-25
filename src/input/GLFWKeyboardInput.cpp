#include "GLFWKeyboardInput.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Engine.h"

GLFWKeyboardInput::GLFWKeyboardInput(GLFWwindow* window)
	: _window(window)
{
	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mode) {
		Engine::GetInstance().GetKeyboardInput()->OnKeyAction(key, action);
		// When a user presses the escape key, we set the WindowShouldClose property to true,
		// closing the application
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	});
}

GLFWKeyboardInput::~GLFWKeyboardInput()
{
	glfwSetKeyCallback(_window, nullptr);
}