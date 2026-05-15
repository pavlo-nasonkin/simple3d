#include "Device3D.h"
#include "camera/Camera.h"
#include "Scene3D.h"

int Device3D::sceenWidth = 800;

int Device3D::sceenHeight = 600;

std::shared_ptr<Scene3D> Device3D::scene3D = nullptr;


Camera* Device3D::camera = nullptr;
const GLfloat* Device3D::view = nullptr;
const GLfloat* Device3D::projection = nullptr;
const GLfloat* Device3D::model = nullptr;

const Pivot3D* Device3D::currentObject = nullptr;

Device3D::Device3D()
{
}

Device3D::~Device3D()
{
}
