#pragma once

#include <GL/glew.h>

// Offscreen render target: color (GL_RGBA8) + depth/stencil. Используется как
// цель рендера сцены для отображения во вьюпорт-панели редактора.
// Требует активного GL-контекста при создании/удалении.
class Framebuffer
{
    GLuint _fbo = 0;
    GLuint _colorTex = 0;
    GLuint _depthRbo = 0;
    int _width = 0;
    int _height = 0;

    void Allocate();

public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // Пересоздаёт вложения под новый размер (no-op, если размер не изменился).
    void Resize(int width, int height);

    // Биндит FBO и ставит viewport под его размер.
    void Bind() const;
    static void Unbind();

    GLuint ColorTexture() const { return _colorTex; }
    int Width() const { return _width; }
    int Height() const { return _height; }
};
