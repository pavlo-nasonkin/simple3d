#pragma once

#include <string>
#include <memory>
#include "Scene3D.h"
class RenderModeHelper;
class UpdateBroadcaster;
class KeyboardInput;
class MouseInput;
class ObjectSelector;
class TextureManager;
struct GLFWwindow;
class Engine
{
	//Private constructor prevents direct instantiation
	Engine();
	~Engine();

public:
	static Engine& GetInstance() {
		static Engine instance;
		return instance;
	}

	// 2. Delete copy constructor and assignment operator to prevent duplicates
	Engine(const Engine&) = delete;
	void operator=(const Engine&) = delete;

	void Init(GLFWwindow* window);
	void Cleanup();

	long long GetCurrentTimeMillis() const;
	double GetTimerSec() const;
	void Log(const std::string& msg);

	const std::shared_ptr<MouseInput>& GetMouseInput() const
	{
		assert(_mouseInput && "Engine::Init must be called first");
		return _mouseInput;
	}
	const std::shared_ptr<KeyboardInput>& GetKeyboardInput() const
	{
		assert(_keyboardInput && "Engine::Init must be called first");
		return _keyboardInput;
	}
	const std::shared_ptr<UpdateBroadcaster>& GetUpdateBroadcaster() const
	{
		assert(_updateBroadcaster && "Engine::Init must be called first");
		return _updateBroadcaster;
	}
	const std::shared_ptr<ObjectSelector>& GetObjectSelector() const
	{
		assert(_objectSelector && "Engine::Init must be called first");
		return _objectSelector;
	}
	const std::unique_ptr<TextureManager>& GetTextureManager() const {
		assert(_textureManager && "Engine::Init must be called first");
		return _textureManager;
	}

	void SetObjectSelector(const std::shared_ptr<ObjectSelector>& value) { _objectSelector = value; }
private:
	std::unique_ptr<TextureManager> _textureManager = nullptr;
	std::unique_ptr<RenderModeHelper> _renderModeHelper = nullptr;
	std::shared_ptr<ObjectSelector> _objectSelector = nullptr;

	std::shared_ptr<MouseInput> _mouseInput = nullptr;
	std::shared_ptr<KeyboardInput> _keyboardInput = nullptr;
	std::shared_ptr<UpdateBroadcaster> _updateBroadcaster = nullptr;

	long long _startTime = 0;
};