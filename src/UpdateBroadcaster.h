#pragma once
#ifndef UPDATE_BROADCASTER_H
#define UPDATE_BROADCASTER_H

//#include "events\IUpdateListener.h"
#include <vector>
class IUpdateListener;

class UpdateBroadcaster
{
private:
	static std::vector<IUpdateListener*> listeners;
public:
	UpdateBroadcaster() {};
	static void addListener(IUpdateListener* listener);
	void update(float deltaTime);
};

#endif // !UPDATE_BROADCASTER_H

