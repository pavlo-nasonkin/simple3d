#include "Pivot3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include "render/MeshRenderer.h"
#include <algorithm>
#include <stdexcept>
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
    // OnDetach для компонентов, пока они ещё живы (отписки и пр.).
    for (auto& behaviour : _behaviours) {
        if (behaviour) {
            behaviour->Detach();
        }
    }
    _behaviours.clear();
    _pendingAdd.clear();
    _pendingRemove.clear();

    _parent.reset();
    RemoveChildren();
}

void Pivot3D::UpdateBehaviours(float deltaTime)
{
    _iteratingBehaviours = true;
    // Итерируемся по индексам живого вектора; структурные изменения во время тика
    // откладываются (см. AddBehaviour/RemoveBehaviourAt), поэтому вектор стабилен.
    for (std::size_t i = 0; i < _behaviours.size(); ++i) {
        Behaviour* behaviour = _behaviours[i].get();
        if (IsPendingRemoval(behaviour)) {
            continue;
        }
        behaviour->Tick(deltaTime);
    }
    _iteratingBehaviours = false;
    FlushBehaviourChanges();
}

void Pivot3D::UpdateSubtree(float deltaTime)
{
    if (!_active) {
        return;
    }
    UpdateBehaviours(deltaTime);
    // Копия списка детей: компонент в OnUpdate может добавить/удалить дочерний узел.
    auto childrenCopy = _children;
    for (auto& child : childrenCopy) {
        child->UpdateSubtree(deltaTime);
    }
}

void Pivot3D::RemoveBehaviourAt(std::size_t index)
{
    if (index >= _behaviours.size()) {
        return;
    }
    Behaviour* behaviour = _behaviours[index].get();
    if (_iteratingBehaviours) {
        _pendingRemove.push_back(behaviour); // удалится после текущего тика
        return;
    }
    behaviour->Detach();
    _behaviours.erase(_behaviours.begin() + static_cast<std::ptrdiff_t>(index));
}

void Pivot3D::FlushBehaviourChanges()
{
    for (Behaviour* toRemove : _pendingRemove) {
        for (std::size_t i = 0; i < _behaviours.size(); ++i) {
            if (_behaviours[i].get() == toRemove) {
                toRemove->Detach();
                _behaviours.erase(_behaviours.begin() + static_cast<std::ptrdiff_t>(i));
                break;
            }
        }
    }
    _pendingRemove.clear();

    for (auto& added : _pendingAdd) {
        _behaviours.push_back(std::move(added));
    }
    _pendingAdd.clear();
}

bool Pivot3D::IsPendingRemoval(const Behaviour* behaviour) const
{
    return std::find(_pendingRemove.begin(), _pendingRemove.end(), behaviour) != _pendingRemove.end();
}

void Pivot3D::Init()
{

}

Behaviour* Pivot3D::AddBehaviour(std::unique_ptr<Behaviour> behaviour) {
	if (!behaviour) {
		return nullptr;
	}
	Behaviour* raw = behaviour.get();
	if (_iteratingBehaviours) {
		_pendingAdd.push_back(std::move(behaviour));
	} else {
		_behaviours.push_back(std::move(behaviour));
	}
	raw->Attach(this);
	return raw;
}

void Pivot3D::RemoveBehaviour(Behaviour* behaviour) {
	if (!behaviour) {
		return;
	}
	for (std::size_t i = 0; i < _behaviours.size(); ++i) {
		if (_behaviours[i].get() == behaviour) {
			RemoveBehaviourAt(i);
			return;
		}
	}
}

Behaviour* Pivot3D::FindBehaviourByType(const std::string& type) const {
	for (const auto& behaviour : _behaviours) {
		if (behaviour && behaviour->GetTypeName() == type) {
			return behaviour.get();
		}
	}
	return nullptr;
}

Pivot3D* Pivot3D::Root() {
	Pivot3D* node = this;
	while (true) {
		auto parent = node->_parent.lock();
		if (!parent) {
			break;
		}
		node = parent.get();
	}
	return node;
}

glm::mat4 Pivot3D::WorldMatrix() const {
	if (auto parent = _parent.lock()) {
		return parent->WorldMatrix() * LocalMatrix();
	}
	return LocalMatrix();
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
	// В shadow-pass узлы, не отбрасывающие тень, пропускаем вместе с поддеревом.
	if (ctx.shadowPass && !_castShadows) {
		return;
	}

	RenderContext context = ctx;

	context.model = ctx.model * LocalMatrix();

	if (_renderer) {
		_renderer->Draw(context, this, material);
	}

    for (auto child : _children)
    {
        child->Render(context, material);
    }
}

void Pivot3D::SetRenderer(std::unique_ptr<MeshRenderer> renderer)
{
	_renderer = std::move(renderer);
}

void Pivot3D::CollectDrawItems(const glm::mat4& parentWorld, bool shadowPass, std::vector<DrawItem>& out) const
{
	if (shadowPass && !_castShadows) {
		return; // поддерево не отбрасывает тень — пропускаем целиком
	}
	const glm::mat4 world = parentWorld * LocalMatrix();
	if (_renderer) {
		out.push_back(DrawItem{ _renderer.get(), this, world });
	}
	for (const auto& child : _children) {
		child->CollectDrawItems(world, shadowPass, out);
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

void Pivot3D::Translate(float x, float y, float z)
{
	_position.x += x;
	_position.y += y;
	_position.z += z;
}

void Pivot3D::Rotate(float x, float y, float z)
{
	_rotation.x += x;
	_rotation.y += y;
	_rotation.z += z;
	// this->_model = glm::rotate(_model, angle, glm::vec3(x, y, z));
}

void Pivot3D::Scale(float x, float y, float z)
{
	_scale.x *= x;
	_scale.y *= y;
	_scale.z *= z;
}

std::shared_ptr<Pivot3D> Pivot3D::GetChildById(unsigned int id, bool recursive /*= true*/)
{
    for (const auto& child : _children)
	{
        if (child->GetId() == id)
		{
			return child;
		}
	}

	if (recursive) {
        for (const auto& child : _children)
		{
			if (const auto& found = child->GetChildById(id, true)) {
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
	// Продвигаем глобальный счётчик за восстановленный id (при загрузке сцены/префаба),
	// чтобы новые узлы не получили id уже существующих → корректный пикинг и NodeRef/BehRef.
	if (id >= _idCounter)
	{
		_idCounter = id + 1;
	}
}

const std::string& Pivot3D::GetName() const
{
    return _name;
}

void Pivot3D::SetName(const std::string &name)
{
    _name = name;
}