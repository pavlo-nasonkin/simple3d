#pragma once

#include "GLEWImporter.h"
#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "events/IMouseListener.h"

class FreeLookCamera: public Camera, IMouseListener
{
public:
	FreeLookCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
		GLfloat yaw = Camera::YAW,
		GLfloat pitch = Camera::PITCH);

	FreeLookCamera(GLfloat posX, GLfloat posY, GLfloat posZ,
		GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw, GLfloat pitch);
	
	~FreeLookCamera() override;
	glm::mat4 GetViewMatrix() override;
	void HandleMouseButton(int button, int action) override;
	void HandleMouseMove(double xpos, double ypos) override;
	void HandleMouseScroll(double xoffset, double yoffset) override;

private:
	void SubscribeToEvents();
	void UnsubscribeFromEvents();

	bool _cameraDrag = false;
	bool _cameraRotate = false;

	double _startX = 0.0;
	double _startY = 0.0;
	double _rotationOrbitRadius = 3.0f;
};