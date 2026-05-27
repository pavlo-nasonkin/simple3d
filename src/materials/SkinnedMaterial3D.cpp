#include "SkinnedMaterial3D.h"
#include "Shader.h"
#include "../models/ExternalModel.h"

SkinnedMaterial3D::SkinnedMaterial3D(const std::string &vertexShaderPath, const std::string &fragmentShaderPath):
    Material3D(vertexShaderPath, fragmentShaderPath)
{

}

SkinnedMaterial3D::~SkinnedMaterial3D()
{

}

void SkinnedMaterial3D::SetBoneTransform(unsigned int Index, const Matrix4f &Transform)
{
    std::string name = std::string("gBones[") + std::to_string(Index) + std::string("]");
    GLuint boneLocation = glGetUniformLocation(_shader->GetProgram(), name.c_str());
    glUniformMatrix4fv(boneLocation, 1, GL_TRUE, (const GLfloat*)Transform.m);
}

void SkinnedMaterial3D::Bind(const RenderContext& ctx, const Mesh* mesh/* = nullptr*/)
{
    Material3D::Bind(ctx, mesh);

    for (unsigned int i = 0 ; i < transforms->size() ; i++) {
        SetBoneTransform(i, transforms->at(i));
    }
}