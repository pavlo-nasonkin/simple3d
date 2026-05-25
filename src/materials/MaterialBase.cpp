#include "MaterialBase.h"
#include "Shader.h"
#include "../models/Mesh.h"
#include "GLEWImporter.h"


unsigned int MaterialBase::_idCounter = 0;
const std::string MaterialBase::MATERIAL_UPDATE_EVENT = "MATERIAL_UPDATE";

CullFaceMode MaterialBase::cullFace() const
{
    return _cullFace;
}

std::shared_ptr<MaterialBase> MaterialBase::clone() const
{
    auto result = std::make_shared<MaterialBase>(*this);
    result->setId(_idCounter);
    _idCounter++;
    return result;
}

void MaterialBase::setId(unsigned int id)
{
    _id = id;
}

void MaterialBase::setShader(const std::shared_ptr<Shader>& shader)
{
    _shader = shader;
}

MaterialBase::MaterialBase(const std::shared_ptr<Shader>& shader)
    :_shader(shader),
      _cullFace(CullFaceMode::back)
{
    _id = _idCounter;
    _idCounter++;
}

MaterialBase::~MaterialBase()
{
    _shader = nullptr;
}

void MaterialBase::build()
{

}

void MaterialBase::bind(const RenderContext& ctx, const Mesh* /*mesh = nullptr*/)
{
    _shader->use();
    switch (_cullFace)
    {
    case CullFaceMode::back:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        break;
    case CullFaceMode::front:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        break;
    default:
        glDisable(GL_CULL_FACE);
        break;

    }
}

void MaterialBase::unbind()
{
}

std::string MaterialBase::name() const
{
    return _name;
}

void MaterialBase::setName(const std::string &name)
{
    _name = name;
}

unsigned int MaterialBase::id() const
{
    return _id;
}

void MaterialBase::setCullFace(const CullFaceMode &cullFace)
{
    _cullFace = cullFace;
}

std::shared_ptr<Shader> MaterialBase::shader() const
{
    return _shader;
}
