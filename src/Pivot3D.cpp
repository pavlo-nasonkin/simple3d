#include "Pivot3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

unsigned int Pivot3D::_idCounter = 0;
const std::string Pivot3DEvent::CHILDREN_CHANGED = "childrenChanged";

Pivot3D::Pivot3D(): _scale(1.0f,1.0f,1.0f),
                    _id(_idCounter++)
{

}


Pivot3D::~Pivot3D()
{
    _parent.reset();
    RemoveChildren();
}

void Pivot3D::Init()
{

}

glm::mat4 Pivot3D::LocalMatrix() const {
	glm::mat4 m(1.0f);
	m = glm::translate(m, _position);
	m = glm::rotate(m, _rotation.x, glm::vec3(1,0,0));
	m = glm::rotate(m, _rotation.y, glm::vec3(0,1,0));
	m = glm::rotate(m, _rotation.z, glm::vec3(0,0,1));
	m = glm::scale(m, _scale);
	return m;
}

void Pivot3D::AddChild(std::shared_ptr<Pivot3D> child)
{
	if (child == nullptr) {
		throw std::invalid_argument("Child is nullptr");
	}

    if (!child->_parent.expired()) {
        child->_parent.lock()->RemoveChild(child);
	}
	_children.push_back(child);
    child->_parent = shared_from_this();
    dispatchEvent(Pivot3DEvent::CHILDREN_CHANGED);
}

void Pivot3D::RemoveChild(std::shared_ptr<Pivot3D> child)
{
    auto removed_iterator = std::remove(_children.begin(), _children.end(), child);
    if (removed_iterator != _children.end())
    {
        _children.erase(removed_iterator, _children.end());
        child->_parent.reset();
        dispatchEvent(Pivot3DEvent::CHILDREN_CHANGED);
    }
}

void Pivot3D::RemoveChildren()
{
    auto empty = _children.empty();
	_children.clear();
    if (!empty)
    {
        dispatchEvent(Pivot3DEvent::CHILDREN_CHANGED);
    }
}

const pivots_list& Pivot3D::Children()
{
    return _children;
}

void Pivot3D::Render(const RenderContext &ctx, MaterialBase* material /*= nullptr*/)
{
	RenderContext context = ctx;

	context.model = ctx.model * LocalMatrix();
    for (auto child : _children)
    {
        child->Render(context, material);
    }
}

void Pivot3D::SetPosition(float x, float y, float z)
{
	_position.x = x;
	_position.y = y;
	_position.z = z;
}

void Pivot3D::SetRotation(float x, float y, float z)
{
	_rotation.x = x;
	_rotation.y = y;
	_rotation.z = z;
}

void Pivot3D::SetScale(float x, float y, float z)
{
	_scale.x = x;
	_scale.y = y;
	_scale.z = z;
}

void Pivot3D::Translate(float /*x*/, float /*y*/, float /*z*/)
{
}

void Pivot3D::Rotate(float x, float y, float z)
{
	_rotation.x += x;
	_rotation.y += y;
	_rotation.z += z;
	// this->_model = glm::rotate(_model, angle, glm::vec3(x, y, z));
}

void Pivot3D::Scale(float /*x*/, float /*y*/, float /*z*/)
{
}

std::shared_ptr<Pivot3D> Pivot3D::GetChildById(unsigned int id, bool recursive /*= true*/)
{
    for (auto child : _children)
	{
        if (child->GetId() == id)
		{
			return child;
		}
	}

	if (recursive) {
        for (auto child : _children)
		{
            auto found = child->GetChildById(id, true);
            if (found) {
				return found;
			}
		}
	}
    return nullptr;
}

std::shared_ptr<Pivot3D> Pivot3D::GetChildAt(unsigned int pos)
{
    if (_children.size() > pos)
    {
        return _children[pos];
    }
    return nullptr;
}

unsigned int Pivot3D::GetId() const
{
	return _id;
}

void Pivot3D::SetId(unsigned int id)
{
	_id = id;
}

std::string Pivot3D::Name() const
{
    return _name;
}

void Pivot3D::SetName(const std::string &name)
{
    _name = name;
}