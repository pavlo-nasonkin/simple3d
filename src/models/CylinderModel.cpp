#include "CylinderModel.h"

#include <cmath>
#include <glm/glm.hpp>

void CylinderModel::GenerateGeometry(std::vector<VertexTypes::Vertex>& vertices,
                                     std::vector<GLuint>& indices) const
{
    constexpr int sectors = 32;
    constexpr float r = 0.5f;
    constexpr float halfH = 0.5f;
    constexpr float PI = 3.14159265359f;

    // ── боковая поверхность ──
    const int sideStart = static_cast<int>(vertices.size());
    for (int j = 0; j <= sectors; ++j) {
        const float u = static_cast<float>(j) / sectors;
        const float theta = u * 2.0f * PI;
        const float c = std::cos(theta);
        const float s = std::sin(theta);
        const glm::vec3 n(c, 0.0f, s);
        const glm::vec3 tangent(-s, 0.0f, c);

        VertexTypes::Vertex top;
        top.Position = glm::vec3(r * c, halfH, r * s);
        top.Normal = n; top.TexCoords = glm::vec2(u, 1.0f); top.Tangent = tangent;
        vertices.push_back(top);

        VertexTypes::Vertex bottom;
        bottom.Position = glm::vec3(r * c, -halfH, r * s);
        bottom.Normal = n; bottom.TexCoords = glm::vec2(u, 0.0f); bottom.Tangent = tangent;
        vertices.push_back(bottom);
    }
    for (int j = 0; j < sectors; ++j) {
        const int a = sideStart + j * 2;       // top j
        const int b = a + 1;                   // bottom j
        const int c = sideStart + (j + 1) * 2; // top j+1
        const int d = c + 1;                   // bottom j+1
        indices.push_back(a); indices.push_back(b); indices.push_back(c);
        indices.push_back(c); indices.push_back(b); indices.push_back(d);
    }

    // ── крышки (top +Y, bottom -Y) ──
    auto addCap = [&](float y, const glm::vec3& normal) {
        const int center = static_cast<int>(vertices.size());
        VertexTypes::Vertex cv;
        cv.Position = glm::vec3(0.0f, y, 0.0f);
        cv.Normal = normal; cv.TexCoords = glm::vec2(0.5f, 0.5f); cv.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        vertices.push_back(cv);

        const int ringStart = static_cast<int>(vertices.size());
        for (int j = 0; j <= sectors; ++j) {
            const float theta = static_cast<float>(j) / sectors * 2.0f * PI;
            const float c = std::cos(theta);
            const float s = std::sin(theta);
            VertexTypes::Vertex v;
            v.Position = glm::vec3(r * c, y, r * s);
            v.Normal = normal;
            v.TexCoords = glm::vec2(c * 0.5f + 0.5f, s * 0.5f + 0.5f);
            v.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            vertices.push_back(v);
        }
        for (int j = 0; j < sectors; ++j) {
            indices.push_back(center);
            indices.push_back(ringStart + j);
            indices.push_back(ringStart + j + 1);
        }
    };
    addCap(halfH, glm::vec3(0.0f, 1.0f, 0.0f));
    addCap(-halfH, glm::vec3(0.0f, -1.0f, 0.0f));
}
