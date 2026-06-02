#include "Engine.h"

#include <iostream>
#include <ostream>
#include <GLFW/glfw3.h>

#include "UpdateBroadcaster.h"
#include "render/RenderModeHelper.h"
#include "render/GeometryRegistry.h"
#include "resources/TextureManager.h"
#include "input/GLFWKeyboardInput.h"
#include "input/GLFWMouseInput.h"
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

Engine::Engine()
{

}

Engine::~Engine() = default;

void Engine::Init(GLFWwindow* window)
{
    if (_mouseInput)
    {
        Cleanup();
    }
    Log("Engine Initialized");
    _updateBroadcaster = std::make_shared<UpdateBroadcaster>();
    _mouseInput = std::make_shared<GLFWMouseInput>(window);
    _keyboardInput = std::make_shared<GLFWKeyboardInput>(window);
    _textureManager = std::make_unique<TextureManager>();
    _geometryRegistry = std::make_unique<GeometryRegistry>();
    _renderModeHelper = std::make_unique<RenderModeHelper>();
    _startTime = GetCurrentTimeMillis();
}

void Engine::Cleanup()
{
    _renderModeHelper = nullptr;
    if (_geometryRegistry) {
        _geometryRegistry->Cleanup();
    }
    _geometryRegistry = nullptr;
    _textureManager = nullptr;
    _objectSelector = nullptr;
    _mouseInput = nullptr;
    _keyboardInput = nullptr;
    _updateBroadcaster = nullptr;
    MaterialBase::ClearProgramCache();
    ShaderFactory::Cleanup();
}

long long Engine::GetCurrentTimeMillis() const
{
#ifdef WIN32
    return GetTickCount();
#else
    timeval t;
    gettimeofday(&t, NULL);

    long long ret = t.tv_sec * 1000 + t.tv_usec / 1000;
    return ret;
#endif
}

double Engine::GetTimerSec() const
{
   return static_cast<double>(GetCurrentTimeMillis() - _startTime) / 1000.0;
}

void Engine::Log(const std::string &msg) {
    std::cout << msg << std::endl;
}

