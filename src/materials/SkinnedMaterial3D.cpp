#include "SkinnedMaterial3D.h"
#include "Shader.h"
#include "models/ExternalModel.h"

SkinnedMaterial3D::SkinnedMaterial3D(const std::string &vertexShaderPath, const std::string &fragmentShaderPath):
    Material3D(vertexShaderPath, fragmentShaderPath)
{

}

void SkinnedMaterial3D::SetBoneTransform(unsigned int index, const Matrix4f &transform)
{
    const auto& shader = GetShader();
    std::string name = std::string("gBones[") + std::to_string(index) + std::string("]");
    GLuint boneLocation = _uniformCache.GetUniformLocation(name);
    glUniformMatrix4fv(boneLocation, 1, GL_TRUE, (const GLfloat*)transform.m);
}

void SkinnedMaterial3D::Bind(const RenderContext& ctx, const Mesh* mesh/* = nullptr*/)
{
    Material3D::Bind(ctx, mesh);
    for (unsigned int i = 0 ; i < transforms->size() ; i++) {
        SetBoneTransform(i, transforms->at(i));
    }
}