#ifndef TEXTUREMAPFILTER_H
#define TEXTUREMAPFILTER_H

#include "materials/Filter3d.h"
#include <string>
#include <memory>
#include "resources/Texture2D.h"

class TextureMapFilter : public Filter3D
{
private:
    static std::string _filterCode;
    std::string _uniformName;
    std::shared_ptr<Texture2D> _texture;
public:
    TextureMapFilter();
    TextureMapFilter(const std::shared_ptr<Texture2D>& texture);
    ~TextureMapFilter();
    void bind(GLuint program) override;
    std::shared_ptr<Texture2D> texture() const;
    void setTexture(const std::shared_ptr<Texture2D>& texture);
};

#endif // TEXTUREMAPFILTER_H
