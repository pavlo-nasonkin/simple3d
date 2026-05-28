#include "TextureFilterBase.h"

#include "resources/Texture2D.h"
#include "utils/StringUtils.h"

TextureFilterBase::TextureFilterBase(const std::shared_ptr<Texture2D>& texture):
    _texture(texture)
{

}

void TextureFilterBase::Init() {
    Filter3D::Init();

    const std::string idStr = std::to_string(_id);
    const std::string samplerIdStr = std::to_string(_nextUniformId);
    _uniformName = "uSampler" + samplerIdStr;

    StringUtils::replace(_code, "{id}", idStr);
    StringUtils::replace(_code, "{uniform_id}", samplerIdStr);
}

void TextureFilterBase::Bind(GLuint program, GLuint firstTextureUnit)
{
    Filter3D::Bind(program, firstTextureUnit);

    glActiveTexture(GL_TEXTURE0 + firstTextureUnit); // Active proper texture unit before binding

    GLint textureLoc = glGetUniformLocation(program, _uniformName.c_str());
    glUniform1i(textureLoc, static_cast<GLint>(firstTextureUnit));
    glBindTexture(GL_TEXTURE_2D, _texture->id);
}

void TextureFilterBase::Unbind(GLuint program, GLuint firstTextureUnit) {
    Filter3D::Unbind(program, firstTextureUnit);

    glActiveTexture(GL_TEXTURE0 + firstTextureUnit);
    glBindTexture(GL_TEXTURE_2D, 0);
}
