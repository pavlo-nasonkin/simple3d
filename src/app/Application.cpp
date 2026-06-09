#include "Application.h"

#include <iostream>
#include <string>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "GLUtils.h"
#include "Engine.h"
#include "Scene3D.h"
#include "Pivot3D.h"
#include "UpdateBroadcaster.h"
#include "camera/Camera.h"
#include "behaviours/CameraBehaviour.h"
#include "behaviours/FlyCameraBehaviour.h"
#include "behaviours/OrbitCameraBehaviour.h"
#include "render/RenderContext.h"
#include "render/Renderer.h"
#include "render/IBLBaker.h"
#include "resources/HDRLoader.h"
#include "models/PlaneModel.h"
#include "models/ExternalModel.h"
#include "object_selector/ObjectSelector.h"
#include "behaviours/RotatorBehaviour.h"
#include "behaviours/DemoBehaviour.h"
#include "behaviours/BehaviourFactory.h"
#include "behaviours/LightComponent.h"
#include "editor/EditorUI.h"
#include "editor/ViewportPanel.h"
#include "editor/HierarchyPanel.h"
#include "editor/InspectorPanel.h"
#include "editor/MaterialsInspectorPanel.h"
#include "editor/MenuBarPanel.h"
#include "editor/EditorConfig.h"
#include "input/MouseInput.h"
#include "scene/SceneSerializer.h"
#include "utils/AssetPaths.h"

Application::Application() = default;
Application::~Application() = default;

int Application::Run(int argc, char** argv)
{
    // Корень ассетов определяем от расположения exe, чтобы запуск из любого CWD
    // (IDE, каталог сборки, инсталляция) находил assets/ одинаково.
    AssetPaths::Init(argc > 0 ? argv[0] : nullptr);

    // Реестр сериализуемых компонентов (нужен до загрузки сцены/префабов).
    BehaviourFactory::RegisterBuiltins();

    if (!InitWindow(_windowWidth, _windowHeight)) {
        return -1;
    }

    Engine::GetInstance().Init(_window);

    SetupScene(argc, argv);
    InitEditor();

    // В полноэкранном режиме пикинг по ЛКМ через MouseInput; в редакторе — через вьюпорт.
    Engine::GetInstance().GetObjectSelector()->SetAutoPick(true);

    MainLoop();

    Shutdown();
    return 0;
}

bool Application::InitWindow(int width, int height)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    _window = glfwCreateWindow(width, height, "simple3d", nullptr, nullptr);
    if (_window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(_window);
    std::cout << "GL VERSION " << glGetString(GL_VERSION) << std::endl;

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    glCheckError();

    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(_window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glCheckError();

    // Размер/аспект камеры следует за вьюпорт-панелью, не за окном — просто обновляем viewport.
    glfwSetFramebufferSizeCallback(_window, [](GLFWwindow*, int w, int h) {
        glViewport(0, 0, w, h);
    });
    return true;
}

Camera* Application::ActiveCamera() const
{
    return _cameraBehaviour ? _cameraBehaviour->GetCamera().get() : nullptr;
}

void Application::SetupScene(int argc, char** argv)
{
    _scene = std::make_shared<Scene3D>();
    _scene->Init();
    _scene->EnableShadows(4096, 20.0f);

    // Камера — узел графа: CameraBehaviour (сенсор) + OrbitCameraBehaviour (СКМ-pan/ПКМ-орбита/колесо-dolly).
    // Контроллер добавляем первым, сенсор — вторым, чтобы Refresh видел уже сдвинутый узел.
    _cameraNode = std::make_shared<Pivot3D>();
    _cameraNode->SetName("MainCamera");
    _cameraNode->SetPosition(0.0f, 0.0f, 3.0f);
    _cameraNode->AddBehaviour<OrbitCameraBehaviour>();
    _cameraBehaviour = _cameraNode->AddBehaviour<CameraBehaviour>(45.0f, 0.1f, 100.0f);
    _cameraBehaviour->SetViewportSize(static_cast<float>(_windowWidth), static_cast<float>(_windowHeight));
    _scene->AddChild(_cameraNode);
    _scene->SetActiveCamera(_cameraBehaviour);

    Engine::GetInstance().SetObjectSelector(std::make_shared<ObjectSelector>(_scene, _cameraBehaviour->GetCamera()));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Сцена на старте: аргумент командной строки либо последняя сцена из конфига.
    std::string startupScene = (argc > 1) ? std::string(argv[1]) : EditorConfig::GetLastScene();
    for (char& sc : startupScene) { if (sc == '\\') sc = '/'; }

    bool sceneLoaded = false;
    if (!startupScene.empty()) {
        sceneLoaded = SceneSerializer::Load(*_scene, *_cameraBehaviour->GetCamera(), startupScene);
        if (sceneLoaded) {
            // Камера сериализуется как legacy Camera; переносим позицию на узел, чтобы
            // следующий Refresh не «сбросил» её. Ориентация камеры пока не восстанавливается
            // (узел хранит Euler-поворот) — будет учтено при сериализации камеры-узла (9.8).
            const glm::vec3 p = _cameraBehaviour->GetCamera()->Position;
            _cameraNode->SetPosition(p.x, p.y, p.z);
        }
    }

    if (!sceneLoaded) {
        BuildDemoScene();
    }
}

void Application::BuildDemoScene()
{
    // ── Демо-сцена по умолчанию (если сцена не загружена) ──
    auto cubemap = HDRLoader::EquirectFileToCubemap(AssetPaths::Resolve("env/citrus_orchard_road_puresky_4k.hdr"));
    if (cubemap) {
        _scene->SetSkybox(cubemap);
        // Прекомпиляция IBL из окружения (один раз) и передача в сцену.
        auto irradiance  = IBLBaker::BakeIrradiance(*cubemap);
        auto prefiltered = IBLBaker::BakePrefiltered(*cubemap);
        auto brdfLUT     = IBLBaker::BakeBRDFLUT();
        _scene->SetEnvironment(irradiance, prefiltered, brdfLUT);
    }

    // Солнце как узел с DirectionalLightComponent (направление = forward узла).
    // Scene3D сам найдёт активный направленный свет в графе; поворот даёт взгляд вниз-вперёд.
    auto sun = std::make_shared<Pivot3D>();
    sun->SetName("Sun");
    sun->SetRotation(glm::radians(-50.0f), glm::radians(-25.0f), 0.0f);
    sun->AddBehaviour<DirectionalLightComponent>();
    _scene->AddChild(sun);

    auto ground = std::make_shared<PlaneModel>();
    ground->SetMaterialDirectory(AssetPaths::Resolve("materials/worn_pavement_uddhdb1fw_4k"));
    ground->SetTiling(12.0f);
    ground->SetRoughnessScale(0.3f);
    ground->Init();
    ground->SetScale(100.0f, 100.0f, 100.0f);
    _scene->AddChild(ground);

    auto car = std::make_shared<ExternalModel>(AssetPaths::Resolve("models/orion-skylark-gt/orion_skylark_gt.fbx"));
    car->SetFlipUVs(true);
    car->Init();
    car->SetScale(0.05f, 0.05f, 0.05f);
    _scene->AddChild(car);

    auto militaryBox = std::make_shared<ExternalModel>(AssetPaths::Resolve("models/military_trenches_storage_crate_wood_worn_01_zjkocdjtq_high/Military_Trenches_Storage_Crate_Wood_Worn_01_zjkocdjtq_High.fbx"));
    militaryBox->SetFlipUVs(true);
    militaryBox->Init();
    militaryBox->SetScale(0.05f, 0.05f, 0.05f);
    militaryBox->SetPosition(6.0f, 0.0f, 6.0f);
    // Смоук-тест каркаса Behaviour: ящик медленно крутится вокруг Y.
    militaryBox->AddBehaviour<RotatorBehaviour>(glm::vec3(0.0f, 1.0f, 0.0f), 0.5f);
    // Демонстрация рефлексии свойств: выбери ящик в редакторе → Inspector покажет поля.
    militaryBox->AddBehaviour<DemoBehaviour>();
    _scene->AddChild(militaryBox);
}

void Application::InitEditor()
{
    _editor = std::make_unique<EditorUI>();
    // ImGui инициализируем ПОСЛЕ установки всех наших GLFW-колбэков,
    // чтобы бэкенд ImGui корректно зацепился к ним.
    _editor->Init(_window);

    _viewport = std::make_unique<ViewportPanel>();
    _hierarchy = std::make_unique<HierarchyPanel>();
    _inspector = std::make_unique<InspectorPanel>();
    _materialsInspector = std::make_unique<MaterialsInspectorPanel>();
    _menuBar = std::make_unique<MenuBarPanel>();
}

void Application::MainLoop()
{
    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();

        const float currentFrame = static_cast<float>(glfwGetTime());
        const float deltaTime = currentFrame - _lastFrame;
        _lastFrame = currentFrame;

        Engine::GetInstance().GetUpdateBroadcaster()->Update(deltaTime);

        // Ctrl+E — переключение режима редактора (по фронту нажатия).
        const bool ctrlDown = glfwGetKey(_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
                            || glfwGetKey(_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        const bool eDown = glfwGetKey(_window, GLFW_KEY_E) == GLFW_PRESS;
        if (ctrlDown && eDown && !_eKeyWasPressed) {
            _editorMode = !_editorMode;
            // В редакторе пикингом управляет вьюпорт (панельные координаты).
            Engine::GetInstance().GetObjectSelector()->SetAutoPick(!_editorMode);
        }
        _eKeyWasPressed = eDown;

        // Тик логики (Behaviour) — в обоих режимах: камера/смоук-компоненты должны
        // работать и в редакторе (навигация под ПКМ). Разделение edit/play (когда
        // часть компонентов не должна тикать в редакторе) — отдельная задача.
        _scene->Update(deltaTime);

        if (_editorMode) {
            RenderEditor();
        } else {
            RenderGame();
        }

        glfwSwapBuffers(_window);
    }
}

void Application::RenderEditor()
{
    _editor->BeginFrame();

    // Чистим default framebuffer (фон окна редактора под панелями ImGui).
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Camera* camera = ActiveCamera();

    // Верхнее меню — до докспейса, чтобы WorkSize учёл его высоту.
    _menuBar->Draw(_scene.get(), camera, &_showHierarchy, &_showInspector, &_showMaterials);

    // Host-докспейс с раскладкой по умолчанию (Hierarchy | Viewport | Inspector).
    _editor->BeginDockspace();

    // Сцена рендерится в FBO и показывается в панели Viewport.
    _viewport->Draw(_scene.get(), camera);
    _hierarchy->Draw(_scene.get(), &_showHierarchy);
    _inspector->Draw(&_showInspector);
    _materialsInspector->Draw(&_showMaterials);

    _editor->DrawDefaultUi();
    _editor->UpdateInputCapture(_viewport->IsHovered());
    _editor->EndFrame();
}

void Application::RenderGame()
{
    // Обычный режим: сцена на весь экран, мышь полностью у камеры.
    Engine::GetInstance().GetMouseInput()->SetMouseCaptured(false);

    Camera* camera = ActiveCamera();
    if (!camera) {
        return;
    }

    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(_window, &fbWidth, &fbHeight);
    if (static_cast<int>(camera->GetScreenWidth()) != fbWidth ||
        static_cast<int>(camera->GetScreenHeight()) != fbHeight) {
        _cameraBehaviour->SetViewportSize(static_cast<float>(fbWidth), static_cast<float>(fbHeight));
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);

    RenderContext ctx;
    ctx.model = glm::mat4(1.0f);
    ctx.camera = camera;
    ctx.view = camera->GetViewMatrix();
    ctx.projection = camera->getProjectionMatrix();
    ctx.scene3D = _scene.get();
    Renderer::RenderScene(*_scene, ctx);
}

void Application::Shutdown()
{
    // Освобождаем GL-владеющие объекты, пока контекст ещё жив.
    if (_editor) {
        _editor->Shutdown();
    }
    _menuBar.reset();
    _materialsInspector.reset();
    _inspector.reset();
    _hierarchy.reset();
    _viewport.reset();
    _editor.reset();

    Engine::GetInstance().SetObjectSelector(nullptr);
    _cameraBehaviour = nullptr;
    _cameraNode.reset();
    _scene.reset();

    Engine::GetInstance().Cleanup();
    glfwTerminate();
}
