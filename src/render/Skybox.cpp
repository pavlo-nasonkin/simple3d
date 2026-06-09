#include "Skybox.h"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "materials/ShaderFactory.h"
#include "resources/TextureCube.h"
#include "utils/AssetPaths.h"

namespace {
    const float kCubeVertices[] = {
        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,
    };
}

Skybox::Skybox(std::shared_ptr<TextureCube> cubemap)
    : _cubemap(std::move(cubemap))
{
    _vao.Bind();
    _vbo.SetData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    GLVertexArray::Unbind();

    const auto& vs = ShaderFactory::GetCompiledShader(GL_VERTEX_SHADER, AssetPaths::Resolve("shaders/skybox.vsh"));
    const auto& fs = ShaderFactory::GetCompiledShader(GL_FRAGMENT_SHADER, AssetPaths::Resolve("shaders/skybox.fsh"));

    GLuint program = glCreateProgram();
    glAttachShader(program, vs.id);
    glAttachShader(program, fs.id);
    glLinkProgram(program);
    _shader = std::make_shared<Shader>(program);

    _viewLoc = glGetUniformLocation(program, "view");
    _projLoc = glGetUniformLocation(program, "projection");
    _samplerLoc = glGetUniformLocation(program, "skybox");
}

void Skybox::Render(const glm::mat4& view, const glm::mat4& projection) const
{
    if (!_cubemap || !_cubemap->Valid()) {
        return;
    }

    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);          // куб смотрим изнутри
    glDepthFunc(GL_LEQUAL);           // пройдут только фрагменты с depth == 1.0

    _shader->Use();

    // убираем перенос из view — небо центрируется на камере
    const glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
    glUniformMatrix4fv(_viewLoc, 1, GL_FALSE, glm::value_ptr(viewNoTranslation));
    glUniformMatrix4fv(_projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glUniform1i(_samplerLoc, 0);
    _cubemap->Bind(0);

    _vao.Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    GLVertexArray::Unbind();

    glDepthFunc(GL_LESS);             // вернуть дефолт
    if (cullWasEnabled) {
        glEnable(GL_CULL_FACE);
    }
}
