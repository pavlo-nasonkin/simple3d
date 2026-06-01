#include "ShadowMap.h"

#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

ShadowMap::ShadowMap(int size)
    : _size(size)
{
    glGenTextures(1, &_depthTex);
    glBindTexture(GL_TEXTURE_2D, _depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, _size, _size, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // вне карты глубина = 1.0 (максимум) => фрагмент всегда «освещён», тени за картой нет
    const float border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTex, 0);
    // depth-only FBO: цветовых буферов нет
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "[ShadowMap] framebuffer incomplete" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowMap::~ShadowMap()
{
    if (_depthTex) glDeleteTextures(1, &_depthTex);
    if (_fbo) glDeleteFramebuffers(1, &_fbo);
}

void ShadowMap::Begin()
{
    glGetIntegerv(GL_VIEWPORT, _prevViewport);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_prevFbo);

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glViewport(0, 0, _size, _size);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::End()
{
    glBindFramebuffer(GL_FRAMEBUFFER, _prevFbo);
    glViewport(_prevViewport[0], _prevViewport[1], _prevViewport[2], _prevViewport[3]);
}

glm::mat4 ShadowMap::ComputeLightSpaceMatrix(const glm::vec3& lightDir,
                                             const glm::vec3& sceneCenter,
                                             float sceneRadius)
{
    const glm::vec3 dir = glm::normalize(lightDir);
    // отступаем от центра против направления света (к «солнцу») на радиус сцены
    const glm::vec3 lightPos = sceneCenter - dir * sceneRadius;
    // up не должен быть коллинеарен направлению (свет строго вниз/вверх)
    const glm::vec3 up = (std::abs(dir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f)
                                                   : glm::vec3(0.0f, 1.0f, 0.0f);

    const glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, up);
    // ортография: куб 2*radius по сторонам, глубина от позиции света до дальней стороны
    const glm::mat4 lightProj = glm::ortho(-sceneRadius, sceneRadius,
                                           -sceneRadius, sceneRadius,
                                           0.0f, 2.0f * sceneRadius);
    return lightProj * lightView;
}
