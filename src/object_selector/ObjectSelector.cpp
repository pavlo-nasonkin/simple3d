#include "ObjectSelector.h"
#include "input/MouseInput.h"
#include <iostream>
#include <algorithm>
#include "Shader.h"
#include "materials/ShaderFactory.h"
#include "materials/ObjectIdMaterial.h"
#include "Scene3D.h"
#include <GLFW/glfw3.h>
#include "events/IObjectSelectorListener.h"
#include <memory>

#include "Engine.h"
#include "camera/Camera.h"
#include "render/Framebuffer.h"
#include "render/Renderer.h"
#include "utils/AssetPaths.h"


ObjectSelector::ObjectSelector(const std::shared_ptr<Scene3D> &scene, const std::shared_ptr<Camera>& camera)
    :_selectedObject(nullptr), _scene(scene), _camera(camera)
{
	Engine::GetInstance().GetMouseInput()->AddListener(this, MouseInput::MOUSE_BUTTON);

    _colorMaterial = std::make_shared<ObjectIdMaterial>(AssetPaths::Resolve("shaders/shader.vsh"), AssetPaths::Resolve("shaders/color.fsh"));
	_colorMaterial->Build();
}

ObjectSelector::~ObjectSelector()
{
	Engine::GetInstance().GetMouseInput()->RemoveListener(this, MouseInput::MOUSE_BUTTON);
}

void ObjectSelector::AddSelectListener(IObjectSelectorListener* listener)
{
	if (std::find(_objectSelectListeners.begin(), _objectSelectListeners.end(), listener) == _objectSelectListeners.end())
	{
		_objectSelectListeners.push_back(listener);
	}
}

void ObjectSelector::RemoveSelectListener(IObjectSelectorListener* listener)
{
	std::erase(_objectSelectListeners, listener);
}

void ObjectSelector::HandleMouseButton(int button, int action)
{
	if (_autoPick && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		PickObject();
	}
}

void ObjectSelector::PickAt(int x, int y, int width, int height)
{
	auto scene3D = _scene.lock();
	auto camera = _camera.lock();
	if (!scene3D || !camera || width <= 0 || height <= 0) {
		return;
	}

	if (!_idFramebuffer) {
		_idFramebuffer = std::make_unique<Framebuffer>(width, height);
	} else {
		_idFramebuffer->Resize(width, height);
	}

	_idFramebuffer->Bind();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RenderContext ctx;
	ctx.camera = camera.get();
	ctx.view = camera->GetViewMatrix();
	ctx.projection = camera->getProjectionMatrix();
	ctx.scene3D = scene3D.get();
	Renderer::RenderScene(*scene3D, ctx, _colorMaterial.get());

	// ImGui-координаты: origin сверху-слева; glReadPixels — снизу-слева.
	unsigned char pixel[3] = {0, 0, 0};
	glReadPixels(x, height - 1 - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
	Framebuffer::Unbind();

	const unsigned int id = (pixel[2] << 16) | (pixel[1] << 8) | pixel[0];
	_selectedObject = (id != 0) ? scene3D->GetChildById(id) : nullptr;
	FireChange(_selectedObject);
}

const std::shared_ptr<Pivot3D>& ObjectSelector::GetSelectedObject()
{
	return _selectedObject;
}

void ObjectSelector::SetSelectedObject(std::shared_ptr<Pivot3D> val)
{
	_selectedObject = val;
}

void ObjectSelector::ReadPixel(void* pixel)
{
	if (auto camera = _camera.lock()) {
		glReadPixels(static_cast<GLint>(Engine::GetInstance().GetMouseInput()->GetMouseX()),
		static_cast<GLint>(camera->GetScreenHeight() - Engine::GetInstance().GetMouseInput()->GetMouseY()),
		1, 1,
		GL_RGB, GL_UNSIGNED_BYTE,
		pixel);
	}
}

void ObjectSelector::PickObject()
{
	// Полноэкранный (game) режим: координаты мыши — в координатах окна, viewport = окно.
	// Рендерим id-pass в offscreen-FBO (как PickAt), а не в default framebuffer —
	// иначе при рендере во время колбэка ввода ловим GL-ошибки и мусорный пиксель.
	auto camera = _camera.lock();
	if (!camera) {
		return;
	}
	const int width = static_cast<int>(camera->GetScreenWidth());
	const int height = static_cast<int>(camera->GetScreenHeight());
	const int x = static_cast<int>(Engine::GetInstance().GetMouseInput()->GetMouseX());
	const int y = static_cast<int>(Engine::GetInstance().GetMouseInput()->GetMouseY());
	PickAt(x, y, width, height);
}

void ObjectSelector::FireChange(std::shared_ptr<Pivot3D> selectedObject)
{
	for (IObjectSelectorListener* listener : _objectSelectListeners)
	{
		listener->handleSelectedObject(selectedObject);
	}
}
