#pragma once
#ifndef PIVOT_3D_H
#define PIVOT_3D_H

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
    virtual void init();
    void addChild(std::shared_ptr<Pivot3D> child);
    void removeChild(std::shared_ptr<Pivot3D> child);
	void removeChildren();
    const pivots_list& children();
//    std::vector<Mesh*>& meshes();
    virtual void render(const RenderContext &ctx, MaterialBase* material = nullptr);
	void setPosition(float x, float y, float z);
	void setRotation(float x, float y, float z);
	void setScale(float x, float y, float z);
	const glm::vec3* getPosition() const { return &_position; }
	const glm::vec3* getRotation() const { return &_rotation; }
	const glm::vec3* getScale() const { return &_scale; }

	void translate(float x, float y, float z);
	void rotate(float x, float y, float z);
	void scale(float x, float y, float z);
	
    std::shared_ptr<Pivot3D> getChildById(unsigned int id, bool recursive = true);
    std::shared_ptr<Pivot3D> getChildAt(unsigned int pos);

	unsigned int getId() const;
	void setId(unsigned int id);

    std::string name() const;
    void setName(const std::string &name);

protected:
	glm::mat4 LocalMatrix() const;
};

#endif // !PIVOT_3D_H
