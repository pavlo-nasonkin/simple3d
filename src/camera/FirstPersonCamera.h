#pragma once
#ifndef FIRST_PERSON_CAMERA_H
#define FIRST_PERSON_CAMERA_H

// Std. Includes
#include <vector>

// GL Includes
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "events/IUpdateListener.h"
#include "input/KeyboardInput.h"
#include "UpdateBroadcaster.h"
#include "events/IMouseListener.h"
#include "input/MouseInput.h"
#include "Camera.h"



// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values

const GLfloat SPEED = 3.0f;
const GLfloat SENSITIVTY = 0.25f;



// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class FirstPersonCamera: public Camera, IUpdateListener, IMouseListener
{
private:
	GLfloat _lastX = 400;
	GLfloat _lastY = 300;
	bool _firstMouse = true;
public:
	
	// Camera options
	GLfloat MovementSpeed;
	GLfloat MouseSensitivity;

	// Constructor with vectors
	FirstPersonCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), 
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
		GLfloat yaw = Camera::YAW, 
		GLfloat pitch = Camera::PITCH) 
		: Camera(position, up, yaw, pitch), MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY)
	{
		UpdateBroadcaster::addListener(this);
		MouseInput::addListener(this, MouseInput::MOUSE_MOVE);
	}
	// Constructor with scalar values
	FirstPersonCamera(GLfloat posX, GLfloat posY, GLfloat posZ, GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw, GLfloat pitch) 
		: Camera(posX, posY, posZ, upX, upY, upZ, yaw, pitch), 
		MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY)
	{
		UpdateBroadcaster::addListener(this);
		MouseInput::addListener(this, MouseInput::MOUSE_MOVE);
	}

	

	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime)
	{
		GLfloat velocity = this->MovementSpeed * deltaTime;
		if (direction == FORWARD)
			this->Position += this->Front * velocity;
		if (direction == BACKWARD)
			this->Position -= this->Front * velocity;
		if (direction == LEFT)
			this->Position -= this->Right * velocity;
		if (direction == RIGHT)
			this->Position += this->Right * velocity;
	}

	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constrainPitch = true)
	{
		xoffset *= this->MouseSensitivity;
		yoffset *= this->MouseSensitivity;

		this->Yaw += xoffset;
		this->Pitch += yoffset;

		// Make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (this->Pitch > 89.0f)
				this->Pitch = 89.0f;
			if (this->Pitch < -89.0f)
				this->Pitch = -89.0f;
		}

		// Update Front, Right and Up Vectors using the updated Euler angles
		this->updateCameraVectors();
	}

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(GLfloat yoffset)
	{
		
		if (this->Zoom >= 1.0f && this->Zoom <= 45.0f) {
			this->Zoom -= yoffset;
			this->ZoomChanged = true;
		}
			
		if (this->Zoom <= 1.0f)
			this->Zoom = 1.0f;
		if (this->Zoom >= 45.0f)
			this->Zoom = 45.0f;
	}

	void handleUpdate(float deltaTime) override
	{
		if (KeyboardInput::isKeyPressed(GLFW_KEY_W))
			ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
		if (KeyboardInput::isKeyPressed(GLFW_KEY_S))
			ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
		if (KeyboardInput::isKeyPressed(GLFW_KEY_A))
			ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
		if (KeyboardInput::isKeyPressed(GLFW_KEY_D))
			ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
	}


	virtual void handleMouseMove(double xpos, double ypos) override
	{
		if (_firstMouse)
		{
			_lastX = xpos;
			_lastY = ypos;
			_firstMouse = false;
		}

		GLfloat xoffset = xpos - _lastX;
		GLfloat yoffset = _lastY - ypos; // Reversed since y-coordinates range from bottom to top
		_lastX = xpos;
		_lastY = ypos;
		
		ProcessMouseMovement(xoffset, yoffset);
	}


};

#endif // !FIRST_PERSON_CAMERA_H