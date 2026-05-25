#pragma once

#include "events/IMouseListener.h"
#include <memory>
#include <vector>
#include "Pivot3D.h"
class ObjectIdMaterial;

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
};