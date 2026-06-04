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
public:
    // CPU-сторона вторичного буфера (хранится для сериализации в префаб).
    struct SecondaryData {
        VertexLayout layout;
        std::vector<std::byte> data;
    };

private:
    GLVertexArray _vao;
    GLBuffer _vbo;
    GLBuffer _ebo;
    std::vector<GLBuffer> _secondaryVbos; // для skinning и пр.
    GLsizei _indicesCount = 0;

    // CPU-копии данных (нужны для запекания геометрии в префаб без re-import).
    VertexLayout _layout;
    std::vector<std::byte> _vertexData;
    std::vector<GLuint> _indices;
    std::vector<SecondaryData> _secondaryData;

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

    // Доступ к CPU-данным для сериализации.
    const VertexLayout& Layout() const { return _layout; }
    const std::vector<std::byte>& VertexData() const { return _vertexData; }
    const std::vector<GLuint>& Indices() const { return _indices; }
    const std::vector<SecondaryData>& SecondaryBuffers() const { return _secondaryData; }
};
