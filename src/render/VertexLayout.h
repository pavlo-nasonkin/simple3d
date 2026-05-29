#pragma once
#include <vector>
#include "VertexAttribute.h"

class VertexLayout
{
public:
    explicit VertexLayout(size_t stride) : _stride(stride) {}

    VertexLayout& Add(VertexAttribute attr) {
        _attributes.push_back(attr);
        return *this;
    }

    // Pre-requisite: target VAO is bound, и VBO для этого layout
    // привязан к GL_ARRAY_BUFFER. Заводит атрибуты в текущем VAO.
    void Apply() const {
        for (const auto& attr : _attributes) {
            glEnableVertexAttribArray(attr.location);
            if (attr.isInteger) {
                glVertexAttribIPointer(attr.location, attr.componentCount, attr.componentType,
                                       static_cast<GLsizei>(_stride),
                                       reinterpret_cast<const GLvoid*>(attr.offset));
            } else {
                glVertexAttribPointer(attr.location, attr.componentCount, attr.componentType,
                                      attr.normalize, static_cast<GLsizei>(_stride),
                                      reinterpret_cast<const GLvoid*>(attr.offset));
            }
        }

        // example:
        // // Vertex Positions
        // glEnableVertexAttribArray(VERTEX_ID_LOCATION);
        // glVertexAttribPointer(VERTEX_ID_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
        // // Vertex Normals
        // glEnableVertexAttribArray(NORMAL_ID_LOCATION);
        // glVertexAttribPointer(NORMAL_ID_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
    }

    size_t GetStride() const { return _stride; }
    const std::vector<VertexAttribute>& GetAttributes() const { return _attributes; }
private:
    std::vector<VertexAttribute> _attributes;
    size_t _stride;
};