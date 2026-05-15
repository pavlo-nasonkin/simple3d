#include "UpdateBroadcaster.h"
#include <algorithm>
#include <iostream>
#include "events/IUpdateListener.h"

std::vector<IUpdateListener*> UpdateBroadcaster::listeners;

void UpdateBroadcaster::addListener(IUpdateListener* listener)
{
	if (std::find(listeners.begin(), listeners.end(), listener) == listeners.end()) {
		listeners.push_back(listener);
	}
}

void UpdateBroadcaster::update(float deltaTime)
{
	for (IUpdateListener* listener : UpdateBroadcaster::listeners)
	{
		listener->handleUpdate(deltaTime);
	}
}
