#pragma once
#ifndef FreeLookCamera_h__
#define FreeLookCamera_h__
#include "GLEWImporter.h"
#include "Camera.h"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "events/IMouseListener.h"

class FreeLookCamera: public Camera, IMouseListener
{
private:
	bool _cameraDrag = false;
	bool _cameraRotate = false;

	double _startX = 0.0;
	double _startY = 0.0;
	double _rotationOrbitRadius = 3.0f;
public:
	FreeLookCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
		GLfloat yaw = Camera::YAW,
		GLfloat pitch = Camera::PITCH);

	FreeLookCamera(GLfloat posX, GLfloat posY, GLfloat posZ,
		GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw, GLfloat pitch);
	
	~FreeLookCamera();
	glm::mat4 GetViewMatrix() override;
	void handleMouseButton(int button, int action) override;
	void handleMouseMove(double xpos, double ypos) override;
	void handleMouseScroll(double xoffset, double yoffset) override;
protected:
private:
	
};

#endif // FreeLookCamera_h__