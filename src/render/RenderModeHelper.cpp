#include "RenderModeHelper.h"
#include "input/KeyboardInput.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

RenderModeHelper::RenderModeHelper()
{
	KeyboardInput::addListener(this);
}

RenderModeHelper::~RenderModeHelper()
{
	//KeyboardInput::removeListener(this);
}

void RenderModeHelper::setWireframeMode(bool value)
{
	_wireframeMode = value;
	_wireframeMode ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RenderModeHelper::handleKeyInput(int key, int action)
{
	if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
		setWireframeMode(!_wireframeMode);
	}
}
