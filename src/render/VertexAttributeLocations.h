#pragma once
#include "GL/glew.h"

namespace VertexAttribLocation {
    // Базовые геометрические — 0..7
    constexpr GLuint POSITION     = 0;
    constexpr GLuint NORMAL       = 1;
    constexpr GLuint TEXCOORD0    = 2;
    constexpr GLuint TANGENT      = 3;
    constexpr GLuint TEXCOORD1    = 4;   // на будущее (lightmaps)
    constexpr GLuint COLOR        = 5;   // на будущее (per-vertex tint)
    // 6, 7 — резерв
    // Skinning — 8..11
    constexpr GLuint BONE_IDS     = 8;
    constexpr GLuint BONE_WEIGHTS = 9;
    // 10, 11 — резерв (например, 8 костей на вершину → BONE_IDS_1, BONE_WEIGHTS_1)
    // Кастомные — 12..15
}
