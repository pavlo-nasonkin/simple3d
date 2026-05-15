
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

    static std::shared_ptr<Scene3D> scene3D;
	static Camera* camera;
	static const GLfloat* view;
	static const GLfloat* projection;
	static const GLfloat* model;

	//TODO think how to remove this global var
	static const Pivot3D* currentObject;


public:
	Device3D();
	~Device3D();
protected:
	
private:
};

#endif // Device3D_h__
