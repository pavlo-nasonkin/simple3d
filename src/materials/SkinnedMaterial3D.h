#ifndef SKINNEDMATERIAL3D_H
#define SKINNEDMATERIAL3D_H
#include "Material3D.h"
#include "utils/Math3d.h"

class SkinnedMaterial3D: public Material3D
{
public:
    std::shared_ptr<std::vector<Matrix4f>> transforms;
public:
    SkinnedMaterial3D(std::shared_ptr<Shader> shader);
    ~SkinnedMaterial3D();
    void SetBoneTransform(unsigned int Index, const Matrix4f& Transform);
    void bind(const Mesh* mesh = nullptr) override;
    std::shared_ptr<MaterialBase> clone() const override;
};

#endif // SKINNEDMATERIAL3D_H
