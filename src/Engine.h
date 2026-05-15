#pragma once
#ifndef Engine_h__
#define Engine_h__

class TextureManager;
class ObjectSelector;

class Engine
{
public:
	static TextureManager* textureManager;
	static ObjectSelector* objectSelector;
public:
	Engine();
    static long long GetCurrentTimeMillis();
    static double getTimerSec();
protected:
	
private:
    static long long startTime;
};

#endif // Engine_h__
