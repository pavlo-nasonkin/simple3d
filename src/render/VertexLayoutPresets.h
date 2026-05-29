#pragma once
#include "VertexAttributeLocations.h"
#include "VertexLayout.h"
#include "VertexTypes.h"

namespace VertexLayouts {
    // Position + Normal + UV + Tangent — стандартная lit-геометрия
    inline VertexLayout Standard() {
        return VertexLayout(sizeof(VertexTypes::Vertex))
            .Add({VertexAttribLocation::POSITION,  3, GL_FLOAT, GL_FALSE, offsetof(VertexTypes::Vertex, Position), false})
            .Add({VertexAttribLocation::NORMAL,    3, GL_FLOAT, GL_FALSE, offsetof(VertexTypes::Vertex, Normal),   false})
            .Add({VertexAttribLocation::TEXCOORD0, 2, GL_FLOAT, GL_FALSE, offsetof(VertexTypes::Vertex, TexCoords), false})
            .Add({VertexAttribLocation::TANGENT,   3, GL_FLOAT, GL_FALSE, offsetof(VertexTypes::Vertex, Tangent),   false});
    }

    // Только позиция — для skybox / debug quads
    inline VertexLayout PositionOnly() {
        return VertexLayout(sizeof(glm::vec3))
            .Add({VertexAttribLocation::POSITION, 3, GL_FLOAT, GL_FALSE, 0, false});
    }

    // Layout для bone-VBO (накладывается на второй buffer)
    inline VertexLayout Skinning() {
        return VertexLayout(sizeof(VertexTypes::VertexBoneData))
            .Add({VertexAttribLocation::BONE_IDS,     4, GL_UNSIGNED_INT,   GL_FALSE, offsetof(VertexTypes::VertexBoneData, IDs),     true})
            .Add({VertexAttribLocation::BONE_WEIGHTS, 4, GL_FLOAT, GL_FALSE, offsetof(VertexTypes::VertexBoneData, Weights), false});
    }

}
