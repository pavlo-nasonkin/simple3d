#pragma once
#ifndef FIRST_PERSON_CAMERA_H
#define FIRST_PERSON_CAMERA_H

// Std. Includes
#include <vector>
#include <cmath>

// GL Includes
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "events/IUpdateListener.h"
#include "input/KeyboardInput.h"
#include "UpdateBroadcaster.h"
#include "events/IMouseListener.h"
#include "input/MouseInput.h"
#include "Engine.h"
#include "Camera.h"



// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values

const GLfloat SPEED = 7.0f;
const GLfloat SENSITIVTY = 0.25f;



// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class FirstPersonCamera: public Camera, IUpdateListener, IMouseListener
{
private:
	bool _firstMouse = true;
	// Цель поворота (накапливается мышью); Yaw/Pitch плавно догоняют её в HandleUpdate.
	GLfloat _targetYaw = Camera::YAW;
	GLfloat _targetPitch = Camera::PITCH;
	GLfloat _rotationSmoothing = 12.0f; // больше = быстрее/жёстче, меньше = плавнее
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
		Engine::GetInstance().GetUpdateBroadcaster()->AddListener(this);
		Engine::GetInstance().GetMouseInput()->AddListener(this, MouseInput::MOUSE_MOVE);
		Engine::GetInstance().GetMouseInput()->SetCursorVisible(false);
		_targetYaw = Yaw;
		_targetPitch = Pitch;
	}
	// Constructor with scalar values
	FirstPersonCamera(GLfloat posX, GLfloat posY, GLfloat posZ, GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw, GLfloat pitch) 
		: Camera(posX, posY, posZ, upX, upY, upZ, yaw, pitch), 
		MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY)
	{
		Engine::GetInstance().GetUpdateBroadcaster()->AddListener(this);
		Engine::GetInstance().GetMouseInput()->AddListener(this, MouseInput::MOUSE_MOVE);
		Engine::GetInstance().GetMouseInput()->SetCursorVisible(false);
		_targetYaw = Yaw;
		_targetPitch = Pitch;
	}

	~FirstPersonCamera() override
	{
		Engine::GetInstance().GetUpdateBroadcaster()->RemoveListener(this);
		Engine::GetInstance().GetMouseInput()->RemoveListener(this, MouseInput::MOUSE_MOVE);
		Engine::GetInstance().GetMouseInput()->SetCursorVisible(true);
	};

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

		// Мышь двигает ЦЕЛЕВЫЕ углы; фактические Yaw/Pitch догоняют их с изингом в HandleUpdate.
		_targetYaw += xoffset;
		_targetPitch += yoffset;

		// Make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (_targetPitch > 89.0f)
				_targetPitch = 89.0f;
			if (_targetPitch < -89.0f)
				_targetPitch = -89.0f;
		}
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

	void HandleUpdate(float deltaTime) override
	{
		// Изинг поворота: экспоненциальное сглаживание, кадронезависимое.
		const GLfloat a = 1.0f - std::exp(-_rotationSmoothing * deltaTime);
		this->Yaw   += (_targetYaw   - this->Yaw)   * a;
		this->Pitch += (_targetPitch - this->Pitch) * a;
		this->updateCameraVectors();

		if (Engine::GetInstance().GetKeyboardInput()->IsKeyPressed(GLFW_KEY_W))
		{
			ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
		}

		if (Engine::GetInstance().GetKeyboardInput()->IsKeyPressed(GLFW_KEY_S))
		{
			ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
		}

		if (Engine::GetInstance().GetKeyboardInput()->IsKeyPressed(GLFW_KEY_A))
		{
			ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
		}

		if (Engine::GetInstance().GetKeyboardInput()->IsKeyPressed(GLFW_KEY_D))
		{
			ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
		}
	}


	void HandleMouseMove(double xpos, double ypos) override
	{
		const GLfloat centerX = _screenWidth * 0.5f;
		const GLfloat centerY = _screenHeight * 0.5f;
		const auto& mouse = Engine::GetInstance().GetMouseInput();

		// На первом событии просто центрируем курсор, чтобы не было резкого скачка.
		if (_firstMouse)
		{
			_firstMouse = false;
			mouse->SetCursorPosition(centerX, centerY);
			return;
		}

		// Смещение считаем относительно центра экрана.
		GLfloat xoffset = static_cast<GLfloat>(xpos) - centerX;
		GLfloat yoffset = centerY - static_cast<GLfloat>(ypos); // Y инвертирован

		if (xoffset != 0.0f || yoffset != 0.0f)
		{
			ProcessMouseMovement(xoffset, yoffset);
			// Возвращаем курсор в центр — следующее смещение снова будет от центра.
			mouse->SetCursorPosition(centerX, centerY);
		}
	}


};

#endif // !FIRST_PERSON_CAMERA_H