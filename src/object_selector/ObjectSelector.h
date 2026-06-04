#pragma once

#include "events/IMouseListener.h"
#include <memory>
#include <vector>
#include "Pivot3D.h"
class ObjectIdMaterial;
class Framebuffer;

class IObjectSelectorListener;

class ObjectSelector: public IMouseListener
{
public:
	ObjectSelector(const std::shared_ptr<Scene3D>& scene, const std::shared_ptr<Camera>& camera);
	~ObjectSelector() override;
	void AddSelectListener(IObjectSelectorListener* listener);
	void RemoveSelectListener(IObjectSelectorListener* listener);

	void HandleMouseButton(int button, int action) override;
    const std::shared_ptr<Pivot3D>& GetSelectedObject();
    void SetSelectedObject(std::shared_ptr<Pivot3D> val);

	// Пикинг по координатам внутри вьюпорт-панели (origin сверху-слева, как у ImGui)
	// размером width x height. Рендерит id-pass в собственный FBO нужного размера.
	void PickAt(int x, int y, int width, int height);
	// Авто-пик по ЛКМ через MouseInput (для полноэкранного режима). В редакторе
	// выключается — там пикингом управляет вьюпорт через PickAt.
	void SetAutoPick(bool enabled) { _autoPick = enabled; }
protected:
	virtual void ReadPixel(void* pixel);
	virtual void PickObject();
private:
    void FireChange(std::shared_ptr<Pivot3D> selectedObject);

	std::shared_ptr<ObjectIdMaterial> _colorMaterial;
	std::shared_ptr<Pivot3D> _selectedObject;
	std::vector<IObjectSelectorListener*> _objectSelectListeners;
	std::weak_ptr<Scene3D> _scene;
	std::weak_ptr<Camera> _camera;
	std::unique_ptr<Framebuffer> _idFramebuffer;
	bool _autoPick = true;
};