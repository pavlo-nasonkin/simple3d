#include "RenderModeHelper.h"
#include "input/KeyboardInput.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Engine.h"

RenderModeHelper::RenderModeHelper()
{
	Engine::GetInstance().GetKeyboardInput()->AddListener(this);
}

RenderModeHelper::~RenderModeHelper()
{
	Engine::GetInstance().GetKeyboardInput()->RemoveListener(this);
}

void RenderModeHelper::SetWireframeMode(bool value)
{
	_wireframeMode = value;
	_wireframeMode ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RenderModeHelper::HandleKeyInput(int key, int action)
{
	if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
		SetWireframeMode(!_wireframeMode);
	}
}
