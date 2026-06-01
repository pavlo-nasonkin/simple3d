#pragma once

#include <GL/glew.h>

// RAII-обёртка над GL cubemap-текстурой (move-only, как GLBuffer/GLVertexArray).
// Требует активного GL-контекста при создании/удалении.
class TextureCube
{
    GLuint _id = 0;
public:
    TextureCube() = default;
    explicit TextureCube(GLuint id) : _id(id) {}
    ~TextureCube();

    TextureCube(const TextureCube&) = delete;
    TextureCube& operator=(const TextureCube&) = delete;
    TextureCube(TextureCube&& other) noexcept : _id(other._id) { other._id = 0; }
    TextureCube& operator=(TextureCube&& other) noexcept;

    GLuint Id() const { return _id; }
    bool Valid() const { return _id != 0; }

    // glActiveTexture(GL_TEXTURE0 + unit) + bind как GL_TEXTURE_CUBE_MAP.
    void Bind(GLuint unit) const;
    static void Unbind();

    // Выделяет пустой cubemap (6 граней) — используется как render-target/IBL-карта.
    // Напр. internalFormat=GL_RGB16F, format=GL_RGB, type=GL_FLOAT.
    static TextureCube CreateEmpty(int faceSize, GLenum internalFormat,
                                   GLenum format, GLenum type, bool withMips);
};
