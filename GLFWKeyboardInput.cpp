#include "GLFWKeyboardInput.h"
#include <algorithm>

#include <Device3D.h>
#include <GLFW/glfw3.h>
#include <BoxModel.h>
#include <Scene3D.h>

GLFWKeyboardInput::GLFWKeyboardInput(GLFWwindow * window)
	:KeyboardInput(), _window(window)
{
	glfwSetKeyCallback(window, keyCallback);
}

GLFWKeyboardInput::~GLFWKeyboardInput()
{
	glfwSetKeyCallback(_window, NULL);
}

void GLFWKeyboardInput::keyCallback(GLFWwindow * window, int key, int scancode, int action, int mode)
{
	KeyboardInput::onKeyAction(key, action);
	// When a user presses the escape key, we set the WindowShouldClose property to true, 
	// closing the application
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		auto box = std::make_shared<BoxModel>();
		Device3D::scene3D->addChild(box);
//		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	
}