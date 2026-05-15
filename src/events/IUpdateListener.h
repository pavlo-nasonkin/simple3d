#pragma once
#ifndef I_UPDATE_LISTENER_H
#define I_UPDATE_LISTENER_H

class IUpdateListener
{
public:
	virtual void handleUpdate(float deltaTime) = 0;
};


#endif // !I_UPDATE_LISTENER_H

