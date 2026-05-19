#pragma once
#ifndef ObjectSelector_h__
#define ObjectSelector_h__
#include "events/IMouseListener.h"
#include <memory>
#include <vector>
#include "Pivot3D.h"
class ObjectIdMaterial;

class IObjectSelectorListener;

class ObjectSelector: public IMouseListener
{
private:
	std::shared_ptr<ObjectIdMaterial> _colorMaterial;
    std::shared_ptr<Pivot3D> _selectedObject;
	std::vector<IObjectSelectorListener*> _objectSelectListeners;
	std::weak_ptr<Scene3D> _scene;
	std::weak_ptr<Camera> _camera;
public:
	ObjectSelector(const std::shared_ptr<Scene3D>& scene, const std::shared_ptr<Camera>& camera);
	virtual ~ObjectSelector();
	void addSelectListener(IObjectSelectorListener* listener);
	void removeSelectListener(IObjectSelectorListener* listener);

	void handleMouseButton(int button, int action) override;
    const std::shared_ptr<Pivot3D>& getSelectedObject();
    void setSelectedObject(std::shared_ptr<Pivot3D> val);
protected:
	virtual void readPixel(void* pixel);
	virtual void pickObject();
private:
    void fireChange(std::shared_ptr<Pivot3D> selectedObject);
};

#endif // ObjectSelector_h__
