#pragma once

#include "Material3D.h"

class SkinnedMaterial3D: public Material3D
{
public:
    std::shared_ptr<std::vector<glm::mat4>> transforms;
public:
    SkinnedMaterial3D(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
    ~SkinnedMaterial3D() override = default;
    void SetBoneTransform(unsigned int index, const glm::mat4& transform);
    void Bind(const RenderContext& ctx, const Pivot3D* node = nullptr) override;
};