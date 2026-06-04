#include "ConeModel.h"

#include <cmath>
#include <glm/glm.hpp>

void ConeModel::GenerateGeometry(std::vector<VertexTypes::Vertex>& vertices,
                                 std::vector<GLuint>& indices) const
{
    constexpr int sectors = 32;
    constexpr float r = 0.5f;
    constexpr float halfH = 0.5f; // основание y=-0.5, вершина y=+0.5 (высота 1)
    constexpr float PI = 3.14159265359f;

    const glm::vec3 apex(0.0f, halfH, 0.0f);

    // ── боковая поверхность (по треугольнику на сектор) ──
    for (int j = 0; j < sectors; ++j) {
        const float t0 = static_cast<float>(j) / sectors * 2.0f * PI;
        const float t1 = static_cast<float>(j + 1) / sectors * 2.0f * PI;
        const float c0 = std::cos(t0), s0 = std::sin(t0);
        const float c1 = std::cos(t1), s1 = std::sin(t1);

        const glm::vec3 p0(r * c0, -halfH, r * s0);
        const glm::vec3 p1(r * c1, -halfH, r * s1);
        // нормаль конуса: горизонталь + вертикаль r/H (H=1)
        const glm::vec3 n0 = glm::normalize(glm::vec3(c0, r, s0));
        const glm::vec3 n1 = glm::normalize(glm::vec3(c1, r, s1));
        const glm::vec3 nApex = glm::normalize(n0 + n1);

        const int base = static_cast<int>(vertices.size());
        VertexTypes::Vertex v0; v0.Position = p0; v0.Normal = n0;
        v0.TexCoords = glm::vec2(static_cast<float>(j) / sectors, 0.0f);
        v0.Tangent = glm::vec3(-s0, 0.0f, c0); vertices.push_back(v0);

        VertexTypes::Vertex v1; v1.Position = p1; v1.Normal = n1;
        v1.TexCoords = glm::vec2(static_cast<float>(j + 1) / sectors, 0.0f);
        v1.Tangent = glm::vec3(-s1, 0.0f, c1); vertices.push_back(v1);

        VertexTypes::Vertex va; va.Position = apex; va.Normal = nApex;
        va.TexCoords = glm::vec2((static_cast<float>(j) + 0.5f) / sectors, 1.0f);
        va.Tangent = glm::vec3(-s0, 0.0f, c0); vertices.push_back(va);

        indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
    }

    // ── основание (-Y) ──
    const int center = static_cast<int>(vertices.size());
    VertexTypes::Vertex cv;
    cv.Position = glm::vec3(0.0f, -halfH, 0.0f);
    cv.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
    cv.TexCoords = glm::vec2(0.5f, 0.5f);
    cv.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    vertices.push_back(cv);

    const int ringStart = static_cast<int>(vertices.size());
    for (int j = 0; j <= sectors; ++j) {
        const float theta = static_cast<float>(j) / sectors * 2.0f * PI;
        const float c = std::cos(theta), s = std::sin(theta);
        VertexTypes::Vertex v;
        v.Position = glm::vec3(r * c, -halfH, r * s);
        v.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
        v.TexCoords = glm::vec2(c * 0.5f + 0.5f, s * 0.5f + 0.5f);
        v.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        vertices.push_back(v);
    }
    for (int j = 0; j < sectors; ++j) {
        indices.push_back(center);
        indices.push_back(ringStart + j);
        indices.push_back(ringStart + j + 1);
    }
}
