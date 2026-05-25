#pragma once

class IUpdateListener
{
public:
	virtual ~IUpdateListener() = default;

	virtual void HandleUpdate(float deltaTime) = 0;
};
