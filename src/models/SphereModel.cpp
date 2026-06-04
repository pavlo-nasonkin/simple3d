#include "SphereModel.h"

#include <cmath>
#include <glm/glm.hpp>

void SphereModel::GenerateGeometry(std::vector<VertexTypes::Vertex>& vertices,
                                   std::vector<GLuint>& indices) const
{
    constexpr int sectors = 32;
    constexpr int stacks = 16;
    constexpr float radius = 0.5f;
    constexpr float PI = 3.14159265359f;

    for (int i = 0; i <= stacks; ++i) {
        const float v = static_cast<float>(i) / stacks;
        const float phi = v * PI;
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);
        for (int j = 0; j <= sectors; ++j) {
            const float u = static_cast<float>(j) / sectors;
            const float theta = u * 2.0f * PI;
            const float sinT = std::sin(theta);
            const float cosT = std::cos(theta);

            const glm::vec3 n(sinPhi * cosT, cosPhi, sinPhi * sinT);
            VertexTypes::Vertex vert;
            vert.Position = radius * n;
            vert.Normal = n;
            vert.TexCoords = glm::vec2(u, v);
            vert.Tangent = glm::vec3(-sinT, 0.0f, cosT);
            vertices.push_back(vert);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            const int k1 = i * (sectors + 1) + j;
            const int k2 = k1 + sectors + 1;
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != stacks - 1) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}
