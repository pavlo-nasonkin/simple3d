
#pragma once
#ifndef Device3D_h__
#define Device3D_h__
#include "GLEWImporter.h"
#include <memory>
#include "Scene3D.h"

class Camera;
class Pivot3D;

class Device3D
{
public: 
	static int sceenWidth;
	static int sceenHeight;

public:
	Device3D();
	~Device3D();
protected:
	
private:
};

#endif // Device3D_h__
