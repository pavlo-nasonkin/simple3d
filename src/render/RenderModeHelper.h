#pragma once
#include "events/IKeyboardListener.h"

class RenderModeHelper: public IKeyboardListener
{
	bool _wireframeMode = false;
public:
	RenderModeHelper();
	~RenderModeHelper() override;
	void SetWireframeMode(bool value);
	void HandleKeyInput(int key, int action) override;
};