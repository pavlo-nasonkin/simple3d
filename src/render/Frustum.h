#pragma once

#include <array>
#include <glm/glm.hpp>

// Frustum из матрицы viewProj (метод Gribb–Hartmann). Плоскости направлены внутрь:
// ax+by+cz+d >= 0 ⇒ точка внутри. Нормализация не нужна для теста AABB «внутри/снаружи».
struct Frustum
{
    std::array<glm::vec4, 6> planes; // L, R, B, T, N, F

    static Frustum FromViewProj(const glm::mat4& m)
    {
        const auto row = [&](int r) { return glm::vec4(m[0][r], m[1][r], m[2][r], m[3][r]); };
        Frustum f;
        f.planes[0] = row(3) + row(0); // left
        f.planes[1] = row(3) - row(0); // right
        f.planes[2] = row(3) + row(1); // bottom
        f.planes[3] = row(3) - row(1); // top
        f.planes[4] = row(3) + row(2); // near
        f.planes[5] = row(3) - row(2); // far
        return f;
    }

    // true, если AABB хотя бы частично внутри (консервативно). mn/mx — мир-координаты.
    bool IntersectsAABB(const glm::vec3& mn, const glm::vec3& mx) const
    {
        for (const glm::vec4& pl : planes) {
            // p-vertex: дальняя точка AABB вдоль нормали плоскости.
            const glm::vec3 p(pl.x >= 0.0f ? mx.x : mn.x,
                              pl.y >= 0.0f ? mx.y : mn.y,
                              pl.z >= 0.0f ? mx.z : mn.z);
            if (pl.x * p.x + pl.y * p.y + pl.z * p.z + pl.w < 0.0f) {
                return false; // полностью снаружи этой плоскости
            }
        }
        return true;
    }
};
