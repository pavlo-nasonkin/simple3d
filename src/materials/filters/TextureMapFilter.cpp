#include "TextureMapFilter.h"
#include "utils/StringUtils.h"

std::string TextureMapFilter::_filterCode =
        "uniform sampler2D uSampler{id};\n"
        "vec4 textureMapFilter{id}()\n"
        "{\n"
        "    return texture(uSampler{id}, TexCoords);\n"
        "}\n";

TextureMapFilter::TextureMapFilter():
    Filter3D(),
    _uniformName()
{
    _code = _filterCode;
    _name = "textureMapFilter";
    const std::string idStr = std::to_string(_id);
    _generatedUniqueName = _name + idStr;
    _uniformName = "uSampler" + idStr;
    StringUtils::replace(_code, "{id}", idStr);
}

TextureMapFilter::TextureMapFilter(const std::shared_ptr<Texture2D>& texture):
    Filter3D(),
    _uniformName(),
    _texture(texture)
{
    //TODO remove CopyPaste code
    _code = _filterCode;
    _name = "textureMapFilter";
    const std::string idStr = std::to_string(_id);
    _generatedUniqueName = _name + idStr;
    _uniformName = "uSampler" + idStr;
    StringUtils::replace(_code, "{id}", idStr);
}

TextureMapFilter::~TextureMapFilter()
{

}

void TextureMapFilter::bind(GLuint program)
{
    Filter3D::bind(program);

    glActiveTexture(GL_TEXTURE0 + 0); // Active proper texture unit before binding
                                      // Retrieve texture number (the N in diffuse_textureN)

    //TODO make _uniformName c_str by default
    GLint textureLoc = glGetUniformLocation(program, _uniformName.c_str());
    glUniform1i(textureLoc, 0);
    glBindTexture(GL_TEXTURE_2D, _texture->id);
}

std::shared_ptr<Texture2D> TextureMapFilter::texture() const
{
    return _texture;
}

void TextureMapFilter::setTexture(const std::shared_ptr<Texture2D>& texture)
{
    _texture = texture;
}
