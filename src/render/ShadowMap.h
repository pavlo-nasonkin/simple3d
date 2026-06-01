#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

// Depth-only FBO для directional shadow mapping.
// Begin() переключает рендер в карту глубины, End() возвращает прежний target.
// Требует активного GL-контекста при создании/удалении.
class ShadowMap
{
    GLuint _fbo = 0;
    GLuint _depthTex = 0;
    int _size = 2048;

    // сохранённое состояние для восстановления в End()
    GLint _prevViewport[4] = {0, 0, 0, 0};
    GLint _prevFbo = 0;

public:
    explicit ShadowMap(int size = 2048);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    // Биндит depth-FBO, ставит viewport в размер карты и чистит глубину.
    void Begin();
    // Восстанавливает предыдущий FBO и viewport.
    void End();

    GLuint DepthTexture() const { return _depthTex; }
    int Size() const { return _size; }

    // Матрица world -> light-clip для directional света.
    //  lightDir    — направление «куда светит» (как dirLight.direction).
    //  sceneCenter — центр области, накрываемой картой теней.
    //  sceneRadius — половина размера ортографического фрустума (тайтнее = выше детализация теней).
    static glm::mat4 ComputeLightSpaceMatrix(const glm::vec3& lightDir,
                                             const glm::vec3& sceneCenter = glm::vec3(0.0f),
                                             float sceneRadius = 20.0f);
};
