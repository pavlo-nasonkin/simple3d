#include "UpdateBroadcaster.h"
#include <algorithm>
#include <iostream>
#include "events/IUpdateListener.h"

void UpdateBroadcaster::AddListener(IUpdateListener* listener)
{
	if (std::find(_listeners.begin(), _listeners.end(), listener) == _listeners.end()) {
		_listeners.push_back(listener);
	}
}

void UpdateBroadcaster::RemoveListener(IUpdateListener *listener) {
	std::erase(_listeners, listener);
}

void UpdateBroadcaster::Update(float deltaTime)
{
	auto listCopy = _listeners;
	for (auto& listener : listCopy)
	{
		listener->HandleUpdate(deltaTime);
	}
}
