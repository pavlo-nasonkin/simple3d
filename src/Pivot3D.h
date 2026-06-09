#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "events/EventDispatcher.h"
#include <memory>
#include <type_traits>
#include "render/RenderContext.h"
#include "render/DrawItem.h"
#include "behaviours/Behaviour.h"
class Shader;
class Material3D;
class MaterialBase;
class MeshRenderer;
class Pivot3D;

typedef std::vector<std::shared_ptr<Pivot3D>>  pivots_list;

struct Pivot3DEvent
{
    static const std::string CHILDREN_CHANGED;
};

class Pivot3D: public EventDispatcher, public std::enable_shared_from_this<Pivot3D>
{
protected:
	static unsigned int _idCounter;
    pivots_list _children;
    std::weak_ptr<Pivot3D> _parent;

protected:
	glm::vec3 _position;
	glm::vec3 _rotation;
	glm::vec3 _scale;
	unsigned int _id;
    std::string _name = "Pivot3D";
	bool _castShadows = true;     // попадает ли узел в shadow-pass (отбрасывает тень)
	bool _receiveShadows = true;  // затеняется ли узел в основном проходе
public:
	Pivot3D();
	~Pivot3D() override;
    virtual void Init();
    void AddChild(std::shared_ptr<Pivot3D> child);
    void RemoveChild(std::shared_ptr<Pivot3D> child);
	void RemoveChildren();
    const pivots_list& Children();
    virtual void Render(const RenderContext &ctx, MaterialBase* material = nullptr);
	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z);
	void SetScale(float x, float y, float z);
	const glm::vec3* GetPosition() const { return &_position; }
	const glm::vec3* GetRotation() const { return &_rotation; }
	const glm::vec3* GetScale() const { return &_scale; }

	void Translate(float x, float y, float z);
	void Rotate(float x, float y, float z);
	void Scale(float x, float y, float z);
	
    std::shared_ptr<Pivot3D> GetChildById(unsigned int id, bool recursive = true);
    std::shared_ptr<Pivot3D> GetChildAt(unsigned int pos);

	unsigned int GetId() const;
	void SetId(unsigned int id);

    const std::string& GetName() const;
    void SetName(const std::string &name);

	bool GetCastShadows() const { return _castShadows; }
	void SetCastShadows(bool value) { _castShadows = value; }
	bool GetReceiveShadows() const { return _receiveShadows; }
	void SetReceiveShadows(bool value) { _receiveShadows = value; }

	// Активность узла: неактивное поддерево пропускается при обновлении (и позже — рендере).
	bool IsActive() const { return _active; }
	void SetActive(bool value) { _active = value; }

	// ── Behaviours (компоненты логики) ───────────────────────────────────────
	// Создаёт компонент типа T на узле и возвращает «сырой» указатель для настройки.
	template <typename T, typename... Args>
	T* AddBehaviour(Args&&... args)
	{
		static_assert(std::is_base_of_v<Behaviour, T>, "T must derive from Behaviour");
		auto behaviour = std::make_unique<T>(std::forward<Args>(args)...);
		T* raw = behaviour.get();
		if (_iteratingBehaviours) {
			_pendingAdd.push_back(std::move(behaviour)); // добавится после текущего тика
		} else {
			_behaviours.push_back(std::move(behaviour));
		}
		raw->Attach(this);
		return raw;
	}

	// Добавляет уже созданный компонент (для фабрики/десериализации).
	Behaviour* AddBehaviour(std::unique_ptr<Behaviour> behaviour);

	// Первый компонент типа T (или производного) либо nullptr.
	template <typename T>
	T* GetBehaviour() const
	{
		for (const auto& behaviour : _behaviours) {
			if (auto* typed = dynamic_cast<T*>(behaviour.get())) {
				return typed;
			}
		}
		return nullptr;
	}

	template <typename T>
	bool HasBehaviour() const { return GetBehaviour<T>() != nullptr; }

	// Доступ к компонентам узла (для инспектора/сериализации).
	const std::vector<std::unique_ptr<Behaviour>>& GetBehaviours() const { return _behaviours; }
	// Первый компонент с данным GetTypeName() (для резолва BehRef), иначе nullptr.
	Behaviour* FindBehaviourByType(const std::string& type) const;

	// Удаляет первый компонент типа T. Во время тика удаление откладывается.
	template <typename T>
	void RemoveBehaviour()
	{
		for (std::size_t i = 0; i < _behaviours.size(); ++i) {
			if (dynamic_cast<T*>(_behaviours[i].get())) {
				RemoveBehaviourAt(i);
				return;
			}
		}
	}

	// Удаляет конкретный компонент по указателю (для инспектора/рантайма).
	void RemoveBehaviour(Behaviour* behaviour);

	// Тик компонентов этого узла (OnStart один раз + OnUpdate). Обычно вызывается
	// из UpdateSubtree, но доступен и отдельно.
	void UpdateBehaviours(float deltaTime);

	// Рекурсивно обновляет компоненты поддерева (неактивные ветви пропускаются).
	void UpdateSubtree(float deltaTime);

	// Мировая матрица узла (произведение локальных матриц вверх по иерархии).
	glm::mat4 WorldMatrix() const;

	// Корень иерархии (верхний предок, обычно Scene3D). Для резолва ссылок по id.
	Pivot3D* Root();

	// Рендер-компонент узла (геометрия+материал). Узел без него — просто transform.
	void SetRenderer(std::unique_ptr<MeshRenderer> renderer);
	MeshRenderer* GetRenderer() const { return _renderer.get(); }

	// Собирает рендер-элементы поддерева в очередь (вместо немедленной отрисовки).
	// В shadowPass поддерево узлов без _castShadows пропускается.
	void CollectDrawItems(const glm::mat4& parentWorld, bool shadowPass, std::vector<DrawItem>& out) const;

protected:
	virtual glm::mat4 LocalMatrix() const;

private:
	void RemoveBehaviourAt(std::size_t index); // отложенно во время тика, иначе сразу
	void FlushBehaviourChanges();              // применяет отложенные add/remove
	bool IsPendingRemoval(const Behaviour* behaviour) const;

	bool _active = true;
	std::unique_ptr<MeshRenderer> _renderer;
	std::vector<std::unique_ptr<Behaviour>> _behaviours;
	std::vector<std::unique_ptr<Behaviour>> _pendingAdd;
	std::vector<Behaviour*> _pendingRemove;
	bool _iteratingBehaviours = false;
};