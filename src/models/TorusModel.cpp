#include "TorusModel.h"

#include <cmath>
#include <glm/glm.hpp>

void TorusModel::GenerateGeometry(std::vector<VertexTypes::Vertex>& vertices,
                                  std::vector<GLuint>& indices) const
{
    constexpr int rings = 32;   // вокруг главной оси
    constexpr int sides = 24;   // вокруг трубы
    constexpr float majorR = 0.35f;
    constexpr float minorR = 0.15f;
    constexpr float PI = 3.14159265359f;

    for (int i = 0; i <= rings; ++i) {
        const float u = static_cast<float>(i) / rings;
        const float phi = u * 2.0f * PI;
        const float cosPhi = std::cos(phi);
        const float sinPhi = std::sin(phi);
        for (int j = 0; j <= sides; ++j) {
            const float v = static_cast<float>(j) / sides;
            const float theta = v * 2.0f * PI;
            const float cosTheta = std::cos(theta);
            const float sinTheta = std::sin(theta);

            VertexTypes::Vertex vert;
            vert.Position = glm::vec3(
                (majorR + minorR * cosTheta) * cosPhi,
                minorR * sinTheta,
                (majorR + minorR * cosTheta) * sinPhi);
            vert.Normal = glm::vec3(cosTheta * cosPhi, sinTheta, cosTheta * sinPhi);
            vert.TexCoords = glm::vec2(u, v);
            vert.Tangent = glm::vec3(-sinPhi, 0.0f, cosPhi);
            vertices.push_back(vert);
        }
    }

    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < sides; ++j) {
            const int k1 = i * (sides + 1) + j;
            const int k2 = k1 + sides + 1;
            indices.push_back(k1);     indices.push_back(k2);     indices.push_back(k1 + 1);
            indices.push_back(k1 + 1); indices.push_back(k2);     indices.push_back(k2 + 1);
        }
    }
}
