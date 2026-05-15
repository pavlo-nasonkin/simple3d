#ifndef GLFW_KEYBOARD_INPUT_H
#define GLFW_KEYBOARD_INPUT_H

#include <vector>
#include "input/KeyboardInput.h"
struct GLFWwindow;

class GLFWKeyboardInput: public KeyboardInput{
private:
	GLFWwindow* _window;

public:
	GLFWKeyboardInput(GLFWwindow* window);
	~GLFWKeyboardInput();

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
};


#endif // !GLFW_KEYBOARD_INPUT_H
