#pragma once
#include <glm/glm.hpp>

namespace VertexTypes {
    struct Vertex {
        // Position
        glm::vec3 Position;
        // Normal
        glm::vec3 Normal;
        // TexCoords
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
    };

    constexpr int kBonesPerVertex = 4;
    struct VertexBoneData
    {
        unsigned int IDs[kBonesPerVertex];
        float Weights[kBonesPerVertex];
    };
}
