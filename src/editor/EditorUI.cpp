#include "EditorUI.h"

// glew.h обязан идти до любого gl.h (его тянет glfw3.h) — иначе #error в glew.h.
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_internal.h> // DockBuilder*
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>

#include <filesystem>

#include "Engine.h"
#include "input/MouseInput.h"

namespace {
    // Рантайм-файл (пишется ImGui в рабочем каталоге) и дефолтный шаблон в репозитории.
    constexpr const char* kRuntimeIni = "imgui.ini";
    constexpr const char* kDefaultIni = "../imgui.ini";

    // Запрошен сброс раскладки — обрабатывается в BeginDockspace (force-rebuild через DockBuilder).
    bool s_resetLayoutRequested = false;

    void BuildDefaultDockLayout(ImGuiID dockspaceId, const ImVec2& size) {
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, size);

        ImGuiID dockMain = dockspaceId;
        const ImGuiID dockLeft  = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left,  0.20f, nullptr, &dockMain);
        const ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.25f, nullptr, &dockMain);

        ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
        ImGui::DockBuilderDockWindow("Inspector", dockRight);
        ImGui::DockBuilderDockWindow("Materials Inspector", dockRight); // вкладкой к Inspector
        ImGui::DockBuilderDockWindow("Viewport", dockMain);
        ImGui::DockBuilderFinish(dockspaceId);
    }
}

void EditorUI::EnsureDefaultLayout()
{
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(kRuntimeIni, ec) && fs::exists(kDefaultIni, ec)) {
        fs::copy_file(kDefaultIni, kRuntimeIni, fs::copy_options::overwrite_existing, ec);
    }
}

void EditorUI::ResetLayout()
{
    // Reload из файла не передокает уже открытые окна — поэтому форсим перестроение
    // через DockBuilder в следующем BeginDockspace.
    s_resetLayoutRequested = true;
}

void EditorUI::Init(GLFWwindow* window)
{
    // До первого NewFrame (ImGui читает ini лениво в первом кадре).
    EnsureDefaultLayout();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // докинг панелей

    ImGui::StyleColorsDark();

    // install_callbacks=true: ImGui ставит свои GLFW-колбэки и ЦЕПЛЯЕТ к ним
    // уже установленные (наш GLFWMouseInput/Keyboard) — поэтому Init вызывать
    // ПОСЛЕ установки наших колбэков.
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    _initialized = true;
}

void EditorUI::Shutdown()
{
    if (!_initialized) {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    _initialized = false;
}

void EditorUI::BeginFrame()
{
    if (!_initialized) {
        return;
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void EditorUI::BeginDockspace()
{
    if (!_initialized) {
        return;
    }

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    const ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##DockHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    const ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");

    // Когда строить дефолтную раскладку:
    //  - первый кадр и нет сохранённой раскладки (imgui.ini её восстановит сам), либо
    //  - запрошен Reset Layout (force-rebuild, чтобы передокать уже открытые окна).
    bool buildLayout = false;
    if (s_resetLayoutRequested) {
        s_resetLayoutRequested = false;
        buildLayout = true;
    } else if (!_dockLayoutInitialized) {
        _dockLayoutInitialized = true;
        buildLayout = (ImGui::DockBuilderGetNode(dockspaceId) == nullptr);
    }

    if (buildLayout) {
        BuildDefaultDockLayout(dockspaceId, vp->WorkSize);
    }

    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
}

void EditorUI::UpdateInputCapture(bool viewportInteractive)
{
    if (!_initialized) {
        return;
    }
    // Над панелями ImGui мышь забирает UI; но если курсор над вьюпортом —
    // отдаём мышь камере (вьюпорт ведёт себя как обычное 3D-окно).
    const ImGuiIO& io = ImGui::GetIO();
    const bool captured = io.WantCaptureMouse && !viewportInteractive;
    Engine::GetInstance().GetMouseInput()->SetMouseCaptured(captured);
}

void EditorUI::DrawDefaultUi()
{
    if (!_initialized) {
        return;
    }

    if (ImGui::Begin("simple3d"))
    {
        ImGui::Text("Editor (этап 8.1)");
        ImGui::Separator();
        ImGui::Text("%.1f FPS (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Checkbox("ImGui demo window", &_showDemo);
    }
    ImGui::End();

    if (_showDemo) {
        ImGui::ShowDemoWindow(&_showDemo);
    }
}

void EditorUI::EndFrame()
{
    if (!_initialized) {
        return;
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
