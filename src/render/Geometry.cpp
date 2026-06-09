#include "Geometry.h"

#include <limits>

#include "render/VertexAttributeLocations.h"

Geometry::Geometry(const VertexLayout& layout,
                   std::span<const std::byte> vertexData,
                   std::span<const GLuint> indices)
    : _layout(layout),
      _vertexData(vertexData.begin(), vertexData.end()),
      _indices(indices.begin(), indices.end())
{
    ComputeBounds();

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

    _secondaryData.push_back(SecondaryData{ layout, std::vector<std::byte>(data.begin(), data.end()) });
}

void Geometry::ComputeBounds()
{
    const size_t stride = _layout.GetStride();
    if (stride == 0 || _vertexData.empty()) {
        return;
    }

    const VertexAttribute* posAttr = nullptr;
    for (const VertexAttribute& a : _layout.GetAttributes()) {
        if (a.location == VertexAttribLocation::POSITION &&
            a.componentType == GL_FLOAT && a.componentCount >= 3) {
            posAttr = &a;
            break;
        }
    }
    if (!posAttr) {
        return;
    }

    const size_t count = _vertexData.size() / stride;
    if (count == 0) {
        return;
    }

    glm::vec3 mn(std::numeric_limits<float>::max());
    glm::vec3 mx(std::numeric_limits<float>::lowest());
    for (size_t i = 0; i < count; ++i) {
        const float* p = reinterpret_cast<const float*>(
            _vertexData.data() + i * stride + posAttr->offset);
        const glm::vec3 v(p[0], p[1], p[2]);
        mn = glm::min(mn, v);
        mx = glm::max(mx, v);
    }
    _aabbMin = mn;
    _aabbMax = mx;
    _hasBounds = true;
}

void Geometry::Draw() const
{
    _vao.Bind();
    glDrawElements(GL_TRIANGLES, _indicesCount, GL_UNSIGNED_INT, nullptr);
    GLVertexArray::Unbind();
}
