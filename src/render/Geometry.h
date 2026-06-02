#pragma once

#include <GL/glew.h>
#include <vector>
#include <span>

#include "render/GLBuffer.h"
#include "render/GLVertexArray.h"
#include "render/VertexLayout.h"

// Иммутабельный владелец GL-ресурсов (VAO/VBO/EBO) без transform и иерархии.
// Шарится через std::shared_ptr<Geometry> (одна геометрия — много Mesh-инстансов).
// Move-only (благодаря RAII-обёрткам GLBuffer/GLVertexArray).
class Geometry
{
    GLVertexArray _vao;
    GLBuffer _vbo;
    GLBuffer _ebo;
    std::vector<GLBuffer> _secondaryVbos; // для skinning и пр.
    GLsizei _indicesCount = 0;

public:
    Geometry(const VertexLayout& layout,
             std::span<const std::byte> vertexData,
             std::span<const GLuint> indices);

    // Удобный типизированный конструктор: Geometry(layout, vertices, indices).
    template <typename V>
    Geometry(const VertexLayout& layout,
             const std::vector<V>& vertices,
             const std::vector<GLuint>& indices)
        : Geometry(layout, std::as_bytes(std::span(vertices)), std::span(indices))
    {
    }

    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;
    Geometry(Geometry&&) = default;
    Geometry& operator=(Geometry&&) = default;

    // Доп. буфер (напр. bone-данные), накладывается на тот же VAO своим layout'ом.
    void AddSecondaryBuffer(const VertexLayout& layout, std::span<const std::byte> data);

    void Draw() const;
    GLsizei IndicesCount() const { return _indicesCount; }
};
