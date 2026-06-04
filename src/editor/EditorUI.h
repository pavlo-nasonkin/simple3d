#pragma once

struct GLFWwindow;

// Обёртка интеграции Dear ImGui поверх GLFW + OpenGL3.
// Жизненный цикл: Init (после glewInit и установки GLFW-колбэков) →
// каждый кадр BeginFrame → (рисование панелей) → EndFrame → Shutdown.
class EditorUI
{
    bool _initialized = false;
    bool _showDemo = false;
    bool _dockLayoutInitialized = false;

public:
    void Init(GLFWwindow* window);
    void Shutdown();

    // Если рантайм imgui.ini отсутствует — копирует дефолтный layout из репозитория.
    static void EnsureDefaultLayout();
    // Сбрасывает раскладку к дефолтной (копирует поверх + перезагружает в ImGui).
    static void ResetLayout();

    // Начинает ImGui-кадр.
    void BeginFrame();
    // Host-окно на весь экран с DockSpace. При первом запуске (нет сохранённого
    // imgui.ini-layout) строит раскладку: Hierarchy слева, Viewport центр, Inspector справа.
    void BeginDockspace();
    // Дефолтный UI этапа 8.1 (заглушки панелей + демо). Позже заменится реальными панелями.
    void DrawDefaultUi();
    // Решает, отдавать ли мышь камере. Если курсор над интерактивным вьюпортом —
    // НЕ захватываем (камера получает мышь), даже если ImGui над окном.
    void UpdateInputCapture(bool viewportInteractive);
    // Завершает кадр и рисует ImGui поверх сцены.
    void EndFrame();
};
