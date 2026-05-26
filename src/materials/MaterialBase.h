#pragma once
#ifndef MaterialBase_h__
#define MaterialBase_h__

#include <string>
#include <memory>

#include "../render/RenderContext.h"

class Shader;
class Mesh;

enum CullFaceMode
{
    none,
    back,
    front
};

class MaterialBase
{
public:
    static const std::string MATERIAL_UPDATE_EVENT;
protected:
    static unsigned int _idCounter;
    std::string _name;
    unsigned int _id;
protected:
    std::shared_ptr<Shader> _shader;
    CullFaceMode _cullFace;
public:
    MaterialBase(const std::shared_ptr<Shader>& shader);
	virtual ~MaterialBase();
    virtual void Build();
	virtual void Bind(const RenderContext& ctx, const Mesh* mesh = nullptr);
	virtual void Unbind();
    std::string name() const;
    void setName(const std::string &name);

    unsigned int id() const;
    void setId(unsigned int id);

    std::shared_ptr<Shader> shader() const;
    void setShader(const std::shared_ptr<Shader> &shader);

    void setCullFace(const CullFaceMode& cullFace);
    CullFaceMode cullFace() const;
    virtual std::shared_ptr<MaterialBase> Clone() const;


protected:

private:
};

#endif // MaterialBase_h__
