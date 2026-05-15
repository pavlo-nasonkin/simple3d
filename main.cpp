#include <iostream>
// GLEW
#define GLEW_STATIC
#include "GL/glew.h"

// GLFW
#include <GLFW/glfw3.h>

#include "GLUtils.h"
#include "Shader.h"
#include "camera/FirstPersonCamera.h"

#include "BoxModel.h"
#include "GLFWKeyboardInput.h"
#include "render/RenderModeHelper.h"
#include "GLFWMouseInput.h"
#include "camera/FreeLookCamera.h"
#include "Engine.h"
#include "Scene3D.h"
#include "Device3D.h"
#include "materials/ShaderFactory.h"

#include "ExternalModel.h"


GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame
							

int initWindow();
GLFWwindow* window;
float screenWidth = 800;
float screenHeight = 600;

int main() {

	int initResult = initWindow();
	if (initResult != 0) {
		return initResult;
	}

	Engine engine;
	FreeLookCamera camera1;
	camera1.buildProjectionMatrix(screenWidth, screenHeight, 45.0f, 0.1f, 100.0f);
	auto scene = std::make_shared<Scene3D>(&camera1);
	UpdateBroadcaster updateBroadcaster;
	GLFWMouseInput mouseInput(window);
	GLFWKeyboardInput keyboardInput(window);
	//MouseInput mouseInput();
	//KeyboardInput keyboardInput();
	RenderModeHelper renderModeHelper;
	ObjectSelector objectSelector;
	engine.objectSelector = &objectSelector;

	//TODO move to Material3D
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	
	auto box = std::make_shared<BoxModel>();
	auto box2 = std::make_shared<BoxModel>();
	auto box3 = std::make_shared<BoxModel>();

	box2->setPosition(5.0f, 0.0f, 0.0f);

	box3->setPosition(0.0f, 0.0f, -5.0f);

	//auto soldier = std::make_shared<ExternalModel>("../assets/models/nanosuit/nanosuit.obj");
	//scene.addChild(soldier);
	scene->addChild(box);
	scene->addChild(box2);
	scene->addChild(box3);

	
	
	//main loop
	while (!glfwWindowShouldClose(window))
	{

		glfwPollEvents();

		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		updateBroadcaster.update(deltaTime);

		scene->render();

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}


int initWindow() {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	

	//Creating a window
	float screenWidth = 800;
	float screenHeight = 600;
	window = glfwCreateWindow(screenWidth, screenHeight, "LearnOpenGL", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwMakeContextCurrent(window);
    std::cout << "GL VERSION " << glGetString(GL_VERSION) << std::endl;
	
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}
	
	glCheckError();

	//Set viewport
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	glViewport(0, 0, width, height);
	glCheckError();
	return 0;
}