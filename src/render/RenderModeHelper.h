#pragma once
#ifndef RENDER_MODE_HELPER_H
#define RENDER_MODE_HELPER_H
//#include <events/IKeyboardListener.h>
#include "events/IKeyboardListener.h"

class RenderModeHelper: public IKeyboardListener
{
private:
	bool _wireframeMode = false;
public:
	RenderModeHelper();
	~RenderModeHelper();
	void setWireframeMode(bool value);
	void handleKeyInput(int key, int action);


};


#endif // !RENDER_MODE_HELPER_H
