#include "camera/FreeLookCamera.h"
#include "input/MouseInput.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include "Pivot3D.h"
#include "Engine.h"
#include "object_selector/ObjectSelector.h"

FreeLookCamera::FreeLookCamera(glm::vec3 position /*= glm::vec3(0.0f, 0.0f, 0.0f)*/, 
	glm::vec3 up /*= glm::vec3(0.0f, 1.0f, 0.0f)*/, 
	GLfloat yaw /*= Camera::YAW*/, 
	GLfloat pitch /*= Camera::PITCH*/)
	: Camera(position, up, yaw, pitch)
{
	SubscribeToEvents();
}

FreeLookCamera::FreeLookCamera(GLfloat posX, GLfloat posY, GLfloat posZ, 
	GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw, GLfloat pitch)
	: Camera(posX, posY, posZ, upX, upY, upZ, yaw, pitch)
{
	SubscribeToEvents();
}

FreeLookCamera::~FreeLookCamera() {
	UnsubscribeFromEvents();
}

glm::mat4 FreeLookCamera::GetViewMatrix()
{
	return Camera::GetViewMatrix();
}

void FreeLookCamera::HandleMouseButton(int button, int action)
{
	_startX = Engine::GetInstance().GetMouseInput()->GetMouseX();
	_startY = Engine::GetInstance().GetMouseInput()->GetMouseY();

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
		if (const auto& target = Engine::GetInstance().GetObjectSelector()->GetSelectedObject()) {
            _rotationOrbitRadius = glm::length(*target->GetPosition() - Position);
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

void FreeLookCamera::HandleMouseMove(double xpos, double ypos)
{
	if (_cameraDrag) {
        auto offsetX = static_cast<float>(_startX - xpos);
        auto offsetY = static_cast<float>(_startY - ypos);

		offsetX /= _screenWidth;
		offsetX *= 3;
		offsetY /= _screenHeight;
		offsetY *= 3;
		this->Position += this->Right * offsetX;
		this->Position -= this->Up * offsetY;
		_startX = xpos;
		_startY = ypos;
	}
	else if (_cameraRotate) {

		glm::vec3 fakeTarget = this->Position + (this->Front * static_cast<float>(_rotationOrbitRadius));
		//Pivot3D* target = Engine::objectSelector->getSelectedObject();

        auto xOffset = static_cast<GLfloat>((xpos - _startX) / 4.0);
        auto yOffset = static_cast<GLfloat>((_startY - ypos) / 4.0);
		_startX = xpos;
		_startY = ypos;

		Pitch = Pitch + yOffset;
		Yaw = fmodf(Yaw + xOffset, 360.f);

		glm::mat4 view(1.0f);

		view = glm::translate(view, fakeTarget);
		view = glm::rotate(view, glm::radians(Yaw + 90.0f), glm::vec3(0.f, 1.f, 0.f));
		view = glm::rotate(view, glm::radians(Pitch), glm::vec3(1.f, 0.f, 0.f));
		view = glm::translate(view, glm::vec3(0.f, 0.f, _rotationOrbitRadius)); // add camera.radius to control the distance-from-target

		Right = view[0];
		Up = view[1];
		Front = -view[2]; // minus because OpenGL camera looks towards negative Z.
		Position = view[3];
	}
}

void FreeLookCamera::HandleMouseScroll(double/* xoffset*/, double yoffset)
{
	if (!_cameraDrag) {
		this->Position += this->Front * float(yoffset);
	}
}

void FreeLookCamera::SubscribeToEvents() {
	Engine::GetInstance().GetMouseInput()->AddListener(this, MouseInput::MOUSE_MOVE);
	Engine::GetInstance().GetMouseInput()->AddListener(this, MouseInput::MOUSE_BUTTON);
	Engine::GetInstance().GetMouseInput()->AddListener(this, MouseInput::MOUSE_SCROLL);
}

void FreeLookCamera::UnsubscribeFromEvents() {
	Engine::GetInstance().GetMouseInput()->RemoveListener(this, MouseInput::MOUSE_MOVE);
	Engine::GetInstance().GetMouseInput()->RemoveListener(this, MouseInput::MOUSE_BUTTON);
	Engine::GetInstance().GetMouseInput()->RemoveListener(this, MouseInput::MOUSE_SCROLL);
}

