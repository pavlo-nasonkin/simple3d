#pragma once
#include <memory>

#include "Filter3d.h"


class Texture2D;

class TextureFilterBase : public Filter3D
{
public:
    explicit TextureFilterBase(const std::shared_ptr<Texture2D>& texture);
    ~TextureFilterBase() override = default;

    void Init() override;
    void Bind(GLuint program, GLuint firstTextureUnit) override;

    unsigned int GetUniformsCount() const override { return 1; }

    const std::shared_ptr<Texture2D>& GetTexture() const { return _texture; }
    void SetTexture(const std::shared_ptr<Texture2D>& texture) { _texture = texture; }

protected:
    std::shared_ptr<Texture2D> _texture;
    std::string _uniformName;
};
