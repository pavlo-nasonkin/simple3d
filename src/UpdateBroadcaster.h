#pragma once

#include <vector>
class IUpdateListener;

class UpdateBroadcaster
{
public:
	UpdateBroadcaster() {};
	void AddListener(IUpdateListener* listener);
	void RemoveListener(IUpdateListener* listener);
	void Update(float deltaTime);

private:
	std::vector<IUpdateListener*> _listeners;
};