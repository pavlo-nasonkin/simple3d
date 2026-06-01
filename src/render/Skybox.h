#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "render/GLBuffer.h"
#include "render/GLVertexArray.h"

class TextureCube;
class Shader;

// Рисует cubemap как фон сцены отдельным pass'ом (свой шейдер, не часть ILightingModel).
// Требует активного GL-контекста при создании.
class Skybox
{
    std::shared_ptr<TextureCube> _cubemap;
    GLVertexArray _vao;
    GLBuffer _vbo;
    std::shared_ptr<Shader> _shader;
    int _viewLoc = -1;
    int _projLoc = -1;
    int _samplerLoc = -1;

public:
    explicit Skybox(std::shared_ptr<TextureCube> cubemap);

    void SetCubemap(std::shared_ptr<TextureCube> cubemap) { _cubemap = std::move(cubemap); }

    // Рисуется последним, с depth func LEQUAL. View берётся без translation,
    // чтобы небо «двигалось» вместе с камерой.
    void Render(const glm::mat4& view, const glm::mat4& projection) const;
};
