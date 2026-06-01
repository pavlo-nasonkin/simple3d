#include "TextureCube.h"

TextureCube::~TextureCube()
{
    if (_id) {
        glDeleteTextures(1, &_id);
    }
}

TextureCube& TextureCube::operator=(TextureCube&& other) noexcept
{
    if (this != &other) {
        if (_id) {
            glDeleteTextures(1, &_id);
        }
        _id = other._id;
        other._id = 0;
    }
    return *this;
}

void TextureCube::Bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _id);
}

void TextureCube::Unbind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

TextureCube TextureCube::CreateEmpty(int faceSize, GLenum internalFormat,
                                     GLenum format, GLenum type, bool withMips)
{
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);

    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                     static_cast<GLint>(internalFormat),
                     faceSize, faceSize, 0, format, type, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    withMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (withMips) {
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return TextureCube(id);
}
