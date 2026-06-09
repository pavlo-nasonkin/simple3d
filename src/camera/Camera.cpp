#include "camera/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

const GLfloat Camera::PITCH = 0.0f;
const GLfloat Camera::ZOOM = 45.0f;
const GLfloat Camera::YAW = -90.0f;


Camera::Camera(GLfloat posX, GLfloat posY, GLfloat posZ, 
	GLfloat upX, GLfloat upY, GLfloat upZ, 
	GLfloat yaw, GLfloat pitch)
	: Front(glm::vec3(0.0f, 0.0f, -1.0f)), Zoom(ZOOM)
{
	this->Position = glm::vec3(posX, posY, posZ);
	this->WorldUp = glm::vec3(upX, upY, upZ);
	this->Yaw = yaw;
	this->Pitch = pitch;
	this->updateCameraVectors();
}

Camera::Camera(glm::vec3 position /*= glm::vec3(0.0f, 0.0f, 0.0f)*/, 
	glm::vec3 up /*= glm::vec3(0.0f, 1.0f, 0.0f)*/, 
	GLfloat yaw /*= YAW*/, 
	GLfloat pitch /*= PITCH*/)
	: Front(glm::vec3(0.0f, 0.0f, -1.0f)), Zoom(ZOOM)
{
	this->Position = position;
	this->WorldUp = up;
	this->Yaw = yaw;
	this->Pitch = pitch;
	this->updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
	return glm::lookAt(this->Position, this->Position + this->Front, this->Up);
}

void Camera::buildProjectionMatrix(float fow, float near, float far)
{
	_projectionMatrix = glm::perspective(glm::radians(fow), _screenWidth / _screenHeight, near, far);
}

void Camera::updateCameraVectors()
{
	// Calculate the new Front vector
	glm::vec3 front;
	front.x = cos(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
	front.y = sin(glm::radians(this->Pitch));
	front.z = sin(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
	this->Front = glm::normalize(front);
	// Also re-calculate the Right and Up vector
	this->Right = glm::normalize(glm::cross(this->Front, this->WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	this->Up = glm::normalize(glm::cross(this->Right, this->Front));
}

