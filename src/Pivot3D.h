#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "events/EventDispatcher.h"
#include <memory>
#include "render/RenderContext.h"
class Shader;
class Mesh;
class Material3D;
class MaterialBase;
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
public:
	Pivot3D();
	~Pivot3D() override;
    virtual void Init();
    void AddChild(std::shared_ptr<Pivot3D> child);
    void RemoveChild(std::shared_ptr<Pivot3D> child);
	void RemoveChildren();
    const pivots_list& Children();
//    std::vector<Mesh*>& meshes();
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

protected:
	glm::mat4 LocalMatrix() const;
};