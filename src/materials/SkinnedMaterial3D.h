#pragma once

#include "Material3D.h"
#include "utils/Math3d.h"

class SkinnedMaterial3D: public Material3D
{
public:
    std::shared_ptr<std::vector<Matrix4f>> transforms;
public:
    SkinnedMaterial3D(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
    ~SkinnedMaterial3D() override = default;
    void SetBoneTransform(unsigned int index, const Matrix4f& transform);
    void Bind(const RenderContext& ctx, const Mesh* mesh = nullptr) override;
};