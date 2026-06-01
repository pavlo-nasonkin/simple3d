#include <iostream>
// GLEW
#define GLEW_STATIC
#include "GL/glew.h"

// GLFW
#include <GLFW/glfw3.h>

#include "GLUtils.h"
#include "Shader.h"
#include "camera/FirstPersonCamera.h"

#include "models/BoxModel.h"
#include "render/RenderModeHelper.h"
#include "camera/FreeLookCamera.h"
#include "Engine.h"
#include "Scene3D.h"
#include "materials/ShaderFactory.h"

#include "models/ExternalModel.h"


GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame
							

int initWindow(float w, float h);
GLFWwindow* window;

int main() {

	float screenWidth = 800;
	float screenHeight = 600;

	int initResult = initWindow(screenWidth, screenHeight);
	if (initResult != 0) {
		return initResult;
	}

	Engine::GetInstance().Init(window);

	{
		auto camera = std::make_shared<FreeLookCamera>();
		camera->SetScreenWidth(screenWidth);
		camera->SetScreenHeight(screenHeight);
		camera->buildProjectionMatrix(45.0f, 0.1f, 100.0f);
		camera->Position.z = 3.0f;

		auto scene3D = std::make_shared<Scene3D>();
		scene3D->Init();

		Engine::GetInstance().SetObjectSelector(std::make_shared<ObjectSelector>(scene3D, camera));

		//TODO move to Material3D
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);


		// auto box = std::make_shared<BoxModel>();
		//
		// box->Init();
		// box->SetColor(0xff0000ff);
		// auto box2 = std::make_shared<BoxModel>();
		// box2->SetColor(0x00ff00ff);
		// box2->Init();
		//
		// box2->SetPosition(1.0f, 0.0f, 0.0f);
		// box2->SetScale(0.5f, 0.5f, 0.5f);

		// auto backpack = std::make_shared<ExternalModel>("../assets/models/backpack/backpack.obj");
		// backpack->SetPosition(3.0f, 0.0f, 0.0f);
		// backpack->Init();
		// scene3D->AddChild(backpack);

		// auto shira = std::make_shared<ExternalModel>("../assets/models/shira/Shira_animation.DAE");
		// shira->Init();
		// scene3D->AddChild(shira);

		auto military_box = std::make_shared<ExternalModel>("../assets/models/orion-skylark-gt/orion_skylark_gt.fbx");
		military_box->SetFlipUVs(true);
		military_box->Init();
		military_box->SetScale(0.05f, 0.05f, 0.05f);
		scene3D->AddChild(military_box);

		// auto model = std::make_shared<ExternalModel>("../assets/models/bolete_mushrooms_pdvcb_high/Bolete_Mushrooms_pdvcB_High.fbx");
		// model->SetPosition(5.0f, 0.0f, 0.0f);
		// model->SetFlipUVs(true);
		// model->Init();
		// model->SetScale(0.1f, 0.1f, 0.1f);
		// scene3D->AddChild(model);

		// auto nanosuit = std::make_shared<ExternalModel>("../assets/models/nanosuit/nanosuit.obj");
		// nanosuit->Init();
		// nanosuit->SetScale(.2f, 0.2f, 0.2f);
		// nanosuit->SetPosition(-3.0f, 0.0f, 0.0f);
		// scene3D->AddChild(nanosuit);



		// scene3D->AddChild(box);
		// box->AddChild(box2);

		glfwSetWindowUserPointer(window, camera.get());
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height) {
			auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(w));
			if (cam) {
				cam->SetScreenWidth(static_cast<float>(width));
				cam->SetScreenHeight(static_cast<float>(height));
				cam->buildProjectionMatrix(45.0f, 0.1f, 100.0f);
			}
			glViewport(0, 0, width, height);
		});


		//main loop
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			GLfloat currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			// box->Rotate(0.0f,1 * deltaTime, 0.0f);
			// box2->Rotate(1 * deltaTime,0.0f, 0.0f);


			Engine::GetInstance().GetUpdateBroadcaster()->Update(deltaTime);
			RenderContext ctx;
			ctx.model = glm::mat4(1.0f);
			ctx.camera = camera.get();
			ctx.view = camera->GetViewMatrix();
			ctx.projection = camera->getProjectionMatrix();
			ctx.scene3D = scene3D.get();
			scene3D->Render(ctx);

			glfwSwapBuffers(window);
		}
	}

	Engine::GetInstance().Cleanup();
	glfwTerminate();
	return 0;
}


int initWindow(float w, float h) {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	

	//Creating a window
	window = glfwCreateWindow(w, h, "LearnOpenGL", nullptr, nullptr);
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