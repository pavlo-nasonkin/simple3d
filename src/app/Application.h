#pragma once

#include <memory>

struct GLFWwindow;
class Scene3D;
class Pivot3D;
class Camera;
class CameraBehaviour;
class EditorUI;
class ViewportPanel;
class HierarchyPanel;
class InspectorPanel;
class MaterialsInspectorPanel;
class MenuBarPanel;

// Точка входа приложения: владеет окном, движком, сценой, камерой, редактором и
// главным циклом. main() лишь создаёт Application и вызывает Run().
class Application
{
public:
    Application();
    ~Application(); // вне строки — unique_ptr на неполные типы

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Полный жизненный цикл: окно → движок → сцена → цикл → очистка.
    // Возвращает код выхода процесса.
    int Run(int argc, char** argv);

private:
    bool InitWindow(int width, int height);
    void SetupScene(int argc, char** argv);
    void BuildDemoScene();
    void InitEditor();
    void MainLoop();
    void RenderEditor();
    void RenderGame();
    void Shutdown();

    Camera* ActiveCamera() const; // Camera* активной камеры сцены (или nullptr)

    GLFWwindow* _window = nullptr;
    int _windowWidth = 800;
    int _windowHeight = 600;

    std::shared_ptr<Scene3D> _scene;
    std::shared_ptr<Pivot3D> _cameraNode;     // камера — узел графа
    CameraBehaviour* _cameraBehaviour = nullptr; // сенсор камеры на узле

    // GL-владеющие компоненты редактора — через unique_ptr, чтобы освободить их
    // в Shutdown() (пока жив GL-контекст), а не в деструкторе после glfwTerminate.
    std::unique_ptr<EditorUI> _editor;
    std::unique_ptr<ViewportPanel> _viewport;
    std::unique_ptr<HierarchyPanel> _hierarchy;
    std::unique_ptr<InspectorPanel> _inspector;
    std::unique_ptr<MaterialsInspectorPanel> _materialsInspector;
    std::unique_ptr<MenuBarPanel> _menuBar;

    bool _editorMode = false;     // по умолчанию редактор выключен
    bool _eKeyWasPressed = false; // edge-детект Ctrl+E
    bool _showHierarchy = true;
    bool _showInspector = true;
    bool _showMaterials = true;

    float _lastFrame = 0.0f;
};
