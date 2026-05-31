#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/vector3.h>

inline glm::mat4 ToGlm(const aiMatrix4x4& m) {
    // aiMatrix4x4 row-major: m.a1=row0_col0, m.a2=row0_col1, ...
    // glm::mat4 column-major: первый аргумент = первый столбец
    // Записываем по столбцам — забираем колонками из ai
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,   // column 0
        m.a2, m.b2, m.c2, m.d2,   // column 1
        m.a3, m.b3, m.c3, m.d3,   // column 2
        m.a4, m.b4, m.c4, m.d4    // column 3
    );
}

inline glm::quat ToGlm(const aiQuaternion& q) {
    // glm::quat(w, x, y, z) — w first
    // aiQuaternion(w, x, y, z) — тоже w first
    return glm::quat(q.w, q.x, q.y, q.z);
}

inline glm::vec3 ToGlm(const aiVector3D& v) {
    return glm::vec3(v.x, v.y, v.z);
}