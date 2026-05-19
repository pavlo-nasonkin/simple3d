#pragma once
#ifndef Engine_h__
#define Engine_h__
#include <string>

#include "Scene3D.h"

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
	static void Log(const std::string& msg);
protected:
	
private:
    static long long startTime;
};

#endif // Engine_h__
