#include "Pivot3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include <algorithm>
#include <exception>
#include "Device3D.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

unsigned int Pivot3D::_idCounter = 0;
const std::string Pivot3DEvent::CHILDREN_CHANGED = "childrenChanged";

Pivot3D::Pivot3D(): _scale(1.0f,1.0f,1.0f),
                    _id(_idCounter++)
{
//	_scale.x = 1.0f;
//	_scale.y = 1.0f;
//	_scale.z = 1.0f;
//	_id = _idCounter++;
}


Pivot3D::~Pivot3D()
{
    if (!_parent.expired()) {
        _parent.lock()->removeChild(shared_from_this());
	}
    _parent.reset();
    removeChildren();
}

void Pivot3D::init()
{

}

void Pivot3D::addChild(std::shared_ptr<Pivot3D> child)
{
	if (child == nullptr) {
		throw "Child is nullptr";
	}

    if (!child->_parent.expired()) {
        child->_parent.lock()->removeChild(child);
	}
	_children.push_back(child);
    child->_parent = shared_from_this();
    dispatchEvent(Pivot3DEvent::CHILDREN_CHANGED);
}

void Pivot3D::removeChild(std::shared_ptr<Pivot3D> child)
{
    auto removed_iterator = std::remove(_children.begin(), _children.end(), child);
    if (removed_iterator != _children.end())
    {
        _children.erase(removed_iterator, _children.end());
        child->_parent.reset();
        dispatchEvent(Pivot3DEvent::CHILDREN_CHANGED);
    }
}

void Pivot3D::removeChildren()
{
    auto empty = _children.empty();
	_children.clear();
    if (!empty)
    {
        dispatchEvent(Pivot3DEvent::CHILDREN_CHANGED);
    }
}

const pivots_list& Pivot3D::children()
{
    return _children;
}

void Pivot3D::render(std::shared_ptr<MaterialBase> shader /*= nullptr*/)
{
    _model = glm::mat4(1.0f);

    _model = glm::scale(_model, _scale);

    applyTransformRotation();

    _model = glm::translate(_model, _position);
    Device3D::model = glm::value_ptr(_model);

    for (auto child : _children)
    {
        child->render(shader);
    }
}

void Pivot3D::setPosition(float x, float y, float z)
{
	_position.x = x;
	_position.y = y;
	_position.z = z;
}

void Pivot3D::setRotation(float x, float y, float z)
{
	_rotation.x = x;
	_rotation.y = y;
	_rotation.z = z;
}

void Pivot3D::setScale(float x, float y, float z)
{
	_scale.x = x;
	_scale.y = y;
	_scale.z = z;
}

void Pivot3D::translate(float /*x*/, float /*y*/, float /*z*/)
{
}

void Pivot3D::rotate(float angle, float x, float y, float z)
{
	this->_model = glm::rotate(_model, angle, glm::vec3(x, y, z));
}

void Pivot3D::scale(float /*x*/, float /*y*/, float /*z*/)
{
}

std::shared_ptr<Pivot3D> Pivot3D::getChildById(unsigned int id, bool recursive /*= true*/)
{
    for (auto child : _children)
	{
        if (child->getId() == id)
		{
			return child;
		}
	}

	if (recursive) {
        for (auto child : _children)
		{
            auto found = child->getChildById(id, true);
            if (found) {
				return found;
			}
		}
	}
    return nullptr;
}

std::shared_ptr<Pivot3D> Pivot3D::getChildAt(unsigned int pos)
{
    if (_children.size() > pos)
    {
        return _children[pos];
    }
    return nullptr;
}

unsigned int Pivot3D::getId() const
{
	return _id;
}

void Pivot3D::setId(unsigned int id)
{
	_id = id;
}

std::string Pivot3D::name() const
{
    return _name;
}

void Pivot3D::setName(const std::string &name)
{
    _name = name;
}

void Pivot3D::applyTransformRotation()
{
    _model = glm::rotate(_model, _rotation.x, glm::vec3(1, 0, 0));
    _model = glm::rotate(_model, _rotation.y, glm::vec3(0, 1, 0));
    _model = glm::rotate(_model, _rotation.z, glm::vec3(0, 0, 1));
}

