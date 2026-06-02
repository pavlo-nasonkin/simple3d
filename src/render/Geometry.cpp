#include "Geometry.h"

Geometry::Geometry(const VertexLayout& layout,
                   std::span<const std::byte> vertexData,
                   std::span<const GLuint> indices)
{
    _vao.Bind();
    _vbo.SetData(GL_ARRAY_BUFFER, vertexData.size_bytes(), vertexData.data(), GL_STATIC_DRAW);
    layout.Apply();
    _ebo.SetData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), GL_STATIC_DRAW);
    _indicesCount = static_cast<GLsizei>(indices.size());
    GLVertexArray::Unbind();
}

void Geometry::AddSecondaryBuffer(const VertexLayout& layout, std::span<const std::byte> data)
{
    _vao.Bind();
    GLBuffer& buffer = _secondaryVbos.emplace_back();
    buffer.SetData(GL_ARRAY_BUFFER, data.size_bytes(), data.data(), GL_STATIC_DRAW);
    layout.Apply();
    GLVertexArray::Unbind();
}

void Geometry::Draw() const
{
    _vao.Bind();
    glDrawElements(GL_TRIANGLES, _indicesCount, GL_UNSIGNED_INT, nullptr);
    GLVertexArray::Unbind();
}
