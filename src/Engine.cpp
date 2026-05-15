#include "Engine.h"
#include "resources/TextureManager.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

TextureManager* Engine::textureManager;
long long Engine::startTime = GetCurrentTimeMillis();

ObjectSelector* Engine::objectSelector = nullptr;

Engine::Engine()
{
	textureManager = new TextureManager();
}

long long Engine::GetCurrentTimeMillis()
{
#ifdef WIN32
    return GetTickCount();
#else
    timeval t;
    gettimeofday(&t, NULL);

    long long ret = t.tv_sec * 1000 + t.tv_usec / 1000;
    return ret;
#endif
}

double Engine::getTimerSec()
{
   return (float)((double)Engine::GetCurrentTimeMillis() - (double)startTime) / 1000.0f;
}

