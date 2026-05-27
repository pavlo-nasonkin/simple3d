#ifndef SKINNEDMATERIAL3D_H
#define SKINNEDMATERIAL3D_H
#include "Material3D.h"
#include "utils/Math3d.h"

class SkinnedMaterial3D: public Material3D
{
public:
    std::shared_ptr<std::vector<Matrix4f>> transforms;
public:
    SkinnedMaterial3D(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
    ~SkinnedMaterial3D() override;
    void SetBoneTransform(unsigned int Index, const Matrix4f& Transform);
    void Bind(const RenderContext& ctx, const Mesh* mesh = nullptr) override;
};

#endif // SKINNEDMATERIAL3D_H
