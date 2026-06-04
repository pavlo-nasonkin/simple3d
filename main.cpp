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
#include "resources/HDRLoader.h"
#include "render/IBLBaker.h"
#include "models/PlaneModel.h"
#include "render/ShadowMap.h"
#include "editor/EditorUI.h"
#include "editor/ViewportPanel.h"
#include "editor/HierarchyPanel.h"
#include "editor/InspectorPanel.h"
#include "editor/MaterialsInspectorPanel.h"
#include "editor/MenuBarPanel.h"
#include "editor/EditorConfig.h"
#include "scene/SceneSerializer.h"
#include <string>

GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame
							

int initWindow(float w, float h);
GLFWwindow* window;

int main(int argc, char** argv) {

	float screenWidth = 800;
	float screenHeight = 600;

	int initResult = initWindow(screenWidth, screenHeight);
	if (initResult != 0) {
		return initResult;
	}

	Engine::GetInstance().Init(window);

	{
		// Для редактора используем FreeLookCamera: курсор свободен (drag для обзора),
		// чтобы панели ImGui были кликабельны. FirstPersonCamera прячет/центрирует
		// курсор — несовместимо с UI.
		auto camera = std::make_shared<FreeLookCamera>();
		camera->SetScreenWidth(screenWidth);
		camera->SetScreenHeight(screenHeight);
		camera->buildProjectionMatrix(45.0f, 0.1f, 100.0f);
		camera->Position.z = 3.0f;

		auto scene3D = std::make_shared<Scene3D>();
		scene3D->Init();
		scene3D->EnableShadows(4096, 20.0f);

		Engine::GetInstance().SetObjectSelector(std::make_shared<ObjectSelector>(scene3D, camera));
		
		//TODO move to Material3D
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// Сцена на старте: аргумент командной строки либо последняя сцена из конфига.
		std::string startupScene = (argc > 1) ? std::string(argv[1]) : EditorConfig::GetLastScene();
		for (char& sc : startupScene) { if (sc == '\\') sc = '/'; }
		bool sceneLoaded = false;
		if (!startupScene.empty()) {
			sceneLoaded = SceneSerializer::Load(*scene3D, *camera, startupScene);
		}

		if (!sceneLoaded) {
		// ── Демо-сцена по умолчанию (если сцена не загружена) ──
		auto cubemap = HDRLoader::EquirectFileToCubemap("../assets/env/citrus_orchard_road_puresky_4k.hdr");
		if (cubemap) {
			scene3D->SetSkybox(cubemap);
			// Прекомпиляция IBL из окружения (один раз) и передача в сцену.
			auto irradiance  = IBLBaker::BakeIrradiance(*cubemap);
			auto prefiltered = IBLBaker::BakePrefiltered(*cubemap);
			auto brdfLUT     = IBLBaker::BakeBRDFLUT();
			scene3D->SetEnvironment(irradiance, prefiltered, brdfLUT);
		}

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

		auto ground = std::make_shared<PlaneModel>();
		//Setup and set material
		ground->SetMaterialDirectory("../assets/materials/worn_pavement_uddhdb1fw_4k");
		ground->SetTiling(12.0f);
		ground->SetRoughnessScale(0.3f);
		ground->Init();
		ground->SetScale(100.0f, 100.0f, 100.0f);
		scene3D->AddChild(ground);

		auto car = std::make_shared<ExternalModel>("../assets/models/orion-skylark-gt/orion_skylark_gt.fbx");
		car->SetFlipUVs(true);
		car->Init();
		car->SetScale(0.05f, 0.05f, 0.05f);
		scene3D->AddChild(car);

		auto military_box = std::make_shared<ExternalModel>("../assets/models/military_trenches_storage_crate_wood_worn_01_zjkocdjtq_high/Military_Trenches_Storage_Crate_Wood_Worn_01_zjkocdjtq_High.fbx");
		military_box->SetFlipUVs(true);
		military_box->Init();
		military_box->SetScale(0.05f, 0.05f, 0.05f);
		military_box->SetPosition(6.0f, 0.0f, 6.0f);
		scene3D->AddChild(military_box);
		} // !sceneLoaded

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

		// Размер/аспект камеры теперь следует за вьюпорт-панелью, не за окном.
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height) {
			glViewport(0, 0, width, height);
		});

		// ImGui инициализируем ПОСЛЕ установки всех наших GLFW-колбэков,
		// чтобы бэкенд ImGui корректно зацепился к ним.
		EditorUI editor;
		editor.Init(window);
		ViewportPanel viewport;
		HierarchyPanel hierarchy;
		InspectorPanel inspector;
		MaterialsInspectorPanel materialsInspector;
		MenuBarPanel menuBar;

		bool editorMode = false;       // по умолчанию редактор выключен
		bool eKeyWasPressed = false;   // для edge-детекта Ctrl+E
		// Видимость панелей (View → Panels)
		bool showHierarchy = true;
		bool showInspector = true;
		bool showMaterials = true;
		// В полноэкранном режиме пикинг по ЛКМ через MouseInput; в редакторе — через вьюпорт.
		Engine::GetInstance().GetObjectSelector()->SetAutoPick(true);

		//main loop
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			GLfloat currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			Engine::GetInstance().GetUpdateBroadcaster()->Update(deltaTime);

			// Ctrl+E — переключение режима редактора (по фронту нажатия).
			const bool ctrlDown = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
			                    || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
			const bool eDown = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
			if (ctrlDown && eDown && !eKeyWasPressed) {
				editorMode = !editorMode;
				// В редакторе пикингом управляет вьюпорт (панельные координаты).
				Engine::GetInstance().GetObjectSelector()->SetAutoPick(!editorMode);
			}
			eKeyWasPressed = eDown;

			if (editorMode)
			{
				editor.BeginFrame();

				// Чистим default framebuffer (фон окна редактора под панелями ImGui).
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				// Верхнее меню — до докспейса, чтобы WorkSize учёл его высоту.
				menuBar.Draw(scene3D.get(), camera.get(), &showHierarchy, &showInspector, &showMaterials);

				// Host-докспейс с раскладкой по умолчанию (Hierarchy | Viewport | Inspector).
				editor.BeginDockspace();

				// Сцена рендерится в FBO и показывается в панели Viewport.
				viewport.Draw(scene3D.get(), camera.get());
				hierarchy.Draw(scene3D.get(), &showHierarchy);
				inspector.Draw(&showInspector);
				materialsInspector.Draw(&showMaterials);

				editor.DrawDefaultUi();
				editor.UpdateInputCapture(viewport.IsHovered());
				editor.EndFrame();
			}
			else
			{
				// Обычный режим: сцена на весь экран, мышь полностью у камеры.
				Engine::GetInstance().GetMouseInput()->SetMouseCaptured(false);

				int fbWidth = 0, fbHeight = 0;
				glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
				if (static_cast<int>(camera->GetScreenWidth()) != fbWidth ||
				    static_cast<int>(camera->GetScreenHeight()) != fbHeight) {
					camera->SetScreenWidth(static_cast<float>(fbWidth));
					camera->SetScreenHeight(static_cast<float>(fbHeight));
					camera->buildProjectionMatrix(45.0f, 0.1f, 100.0f);
				}

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, fbWidth, fbHeight);

				RenderContext ctx;
				ctx.model = glm::mat4(1.0f);
				ctx.camera = camera.get();
				ctx.view = camera->GetViewMatrix();
				ctx.projection = camera->getProjectionMatrix();
				ctx.scene3D = scene3D.get();
				scene3D->Render(ctx);
			}

			glfwSwapBuffers(window);
		}

		editor.Shutdown();
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