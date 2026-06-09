#include "SkinnedMaterial3D.h"
#include "glm/gtc/type_ptr.hpp"
#include "models/ExternalModel.h"

SkinnedMaterial3D::SkinnedMaterial3D(const std::string &vertexShaderPath, const std::string &fragmentShaderPath):
    Material3D(vertexShaderPath, fragmentShaderPath)
{

}

void SkinnedMaterial3D::SetBoneTransform(unsigned int index, const glm::mat4& transform)
{
    std::string name = std::string("gBones[") + std::to_string(index) + std::string("]");
    GLuint boneLocation = _uniformCache.GetUniformLocation(name);
    glUniformMatrix4fv(boneLocation, 1, GL_FALSE, glm::value_ptr(transform));
}

void SkinnedMaterial3D::Bind(const RenderContext& ctx, const Pivot3D* node/* = nullptr*/)
{
    Material3D::Bind(ctx, node);
    for (unsigned int i = 0 ; i < transforms->size() ; i++) {
        SetBoneTransform(i, transforms->at(i));
    }
}