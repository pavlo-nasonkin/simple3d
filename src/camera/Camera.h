#pragma once
#ifndef Camera_h__
#define Camera_h__
#include "GLEWImporter.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
	static const GLfloat YAW;
	static const GLfloat PITCH;// = 0.0f;
	static const GLfloat ZOOM;// = 45.0f;
	// Camera Attributes
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	// Euler Angles
	GLfloat Yaw;
	GLfloat Pitch;

	GLfloat Zoom;
	bool ZoomChanged;
private:
	glm::mat4 _projectionMatrix;
public:
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), GLfloat yaw = YAW, GLfloat pitch = PITCH);
	Camera(GLfloat posX, GLfloat posY, GLfloat posZ, GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw, GLfloat pitch);
	virtual ~Camera() = default;
	// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
	virtual glm::mat4 GetViewMatrix();
	virtual void buildProjectionMatrix(float screenW, float screenH, float fow, float near, float far);

	const glm::mat4& getProjectionMatrix() const { return _projectionMatrix; };
protected:
	// Calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors();
	
private:
};

#endif // Camera_h__