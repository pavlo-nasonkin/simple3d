#include "SkinnedMaterial3D.h"
#include "Shader.h"
#include "ExternalModel.h"

SkinnedMaterial3D::SkinnedMaterial3D(std::shared_ptr<Shader> shader):
    Material3D(shader)
{

}

SkinnedMaterial3D::~SkinnedMaterial3D()
{

}

void SkinnedMaterial3D::SetBoneTransform(unsigned int Index, const Matrix4f &Transform)
{
    std::string name = std::string("gBones[") + std::to_string(Index) + std::string("]");
    GLuint boneLocation = glGetUniformLocation(_shader->Program, name.c_str());
    glUniformMatrix4fv(boneLocation, 1, GL_TRUE, (const GLfloat*)Transform.m);
}

void SkinnedMaterial3D::bind(const Mesh* mesh/* = nullptr*/)
{
    Material3D::bind(mesh);

    for (unsigned int i = 0 ; i < transforms->size() ; i++) {
        SetBoneTransform(i, transforms->at(i));
    }
}

std::shared_ptr<MaterialBase> SkinnedMaterial3D::clone() const
{
    auto result = std::make_shared<SkinnedMaterial3D>(SkinnedMaterial3D(*this));
    result->setId(_idCounter);
    _idCounter++;
    auto shaderCopy = std::make_shared<Shader>(Shader(*_shader.get()));
    shaderCopy->Program = 0;
    result->setShader(shaderCopy);

    return result;
}
