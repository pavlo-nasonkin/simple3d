#include "camera/FreeLookCamera.h"
#include "input/MouseInput.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include "Pivot3D.h"
#include "Engine.h"
#include "object_selector/ObjectSelector.h"
#include "Device3D.h"


FreeLookCamera::FreeLookCamera(glm::vec3 position /*= glm::vec3(0.0f, 0.0f, 0.0f)*/, 
	glm::vec3 up /*= glm::vec3(0.0f, 1.0f, 0.0f)*/, 
	GLfloat yaw /*= Camera::YAW*/, 
	GLfloat pitch /*= Camera::PITCH*/)
	: Camera(position, up, yaw, pitch)
{
	MouseInput::addListener(this, MouseInput::MOUSE_MOVE);
	MouseInput::addListener(this, MouseInput::MOUSE_BUTTON);
	MouseInput::addListener(this, MouseInput::MOUSE_SCROLL);
}

FreeLookCamera::FreeLookCamera(GLfloat posX, GLfloat posY, GLfloat posZ, 
	GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw, GLfloat pitch)
	: Camera(posX, posY, posZ, upX, upY, upZ, yaw, pitch)
{
	MouseInput::addListener(this, MouseInput::MOUSE_MOVE);
	MouseInput::addListener(this, MouseInput::MOUSE_BUTTON);
	MouseInput::addListener(this, MouseInput::MOUSE_SCROLL);
}

FreeLookCamera::~FreeLookCamera()
{

}

glm::mat4 FreeLookCamera::GetViewMatrix()
{
	return Camera::GetViewMatrix();
}

void FreeLookCamera::handleMouseButton(int button, int action)
{
	_startX = MouseInput::mouseX();
	_startY = MouseInput::mouseY();

	switch (button)
	{
	case GLFW_MOUSE_BUTTON_MIDDLE: 
	{
		_cameraDrag = action == GLFW_PRESS;
		break;	
	}
	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		_cameraRotate = action == GLFW_PRESS;
        auto target = Engine::objectSelector->getSelectedObject();
        if (target) {
            _rotationOrbitRadius = glm::length(*target->getPosition() - Position);
		}
		else {
			_rotationOrbitRadius = 3.0f;// glm::length(Position);
		}
		break;
	}
	default:
		break;
	}
}

void FreeLookCamera::handleMouseMove(double xpos, double ypos)
{
	if (_cameraDrag) {
        float offsetX = static_cast<float>(_startX - xpos);
        float offsetY = static_cast<float>(_startY - ypos);

		offsetX /= Device3D::sceenWidth;
		offsetX *= 3;
		offsetY /= Device3D::sceenHeight;
		offsetY *= 3;
		this->Position += this->Right * offsetX;
		this->Position -= this->Up * offsetY;
		_startX = xpos;
		_startY = ypos;
	}
	else if (_cameraRotate) {

		glm::vec3 fakeTarget = this->Position + (this->Front * (float)_rotationOrbitRadius);
		//Pivot3D* target = Engine::objectSelector->getSelectedObject();

        GLfloat xoffset = static_cast<GLfloat>((xpos - _startX) / 4.0);
        GLfloat yoffset = static_cast<GLfloat>((_startY - ypos) / 4.0);
		_startX = xpos;
		_startY = ypos;

		Pitch = Pitch + yoffset;
		Yaw = fmodf(Yaw + xoffset, 360.f);

		glm::mat4 view;
		
		
		view = glm::translate(view, fakeTarget);
		view = glm::rotate(view, glm::radians(Yaw + 90.0f), glm::vec3(0.f, 1.f, 0.f));
		view = glm::rotate(view, glm::radians(Pitch), glm::vec3(1.f, 0.f, 0.f));
		view = glm::translate(view, glm::vec3(0.f, 0.f, _rotationOrbitRadius)); // add camera.radius to control the distance-from-target
		

		Right = view[0];
		Up = view[1];
		Front = -view[2]; // minus because OpenGL camera looks towards negative Z.
		Position = view[3];

		view = glm::inverse(view);
	}
}

void FreeLookCamera::handleMouseScroll(double/* xoffset*/, double yoffset)
{
	if (!_cameraDrag) {
		this->Position += this->Front * float(yoffset);
	}
}

