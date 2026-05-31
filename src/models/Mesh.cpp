#include "Mesh.h"
#include "materials/MaterialBase.h"
#include "Pivot3D.h"
#include "GLUtils.h"
#include <glm/gtc/matrix_transform.hpp>

#include "render/VertexLayout.h"

Mesh::Mesh(const std::shared_ptr<MaterialBase>& mat) :
    _material(mat)
{
    assert(mat && "Mesh requires a non-null material");
    _name = "Mesh";
}

// Render the mesh
void Mesh::Render(const RenderContext &ctx, MaterialBase* material)
{
    RenderContext context =  ctx;
    context.model = ctx.model * LocalMatrix();
    if (material == nullptr) {
        material = _material.get();
    }

    if (_indicesCount > 0) {
        material->Bind(context, this);

        _vao.Bind();
        glDrawElements(GL_TRIANGLES, _indicesCount, GL_UNSIGNED_INT, 0);
        GLVertexArray::Unbind();

        material->Unbind();
    }

    for (auto& child : _children) {
        child->Render(context, material);
    }
}

/*  Functions    */
// Initializes all the buffer objects/arrays

void Mesh::SetupMesh(const VertexLayout& vertexLayout, std::span<const std::byte> vertexData, std::span<const GLuint> indices)
{
    _vao.Bind();
    {
        // Load data into vertex buffers
        _vbo.SetData(GL_ARRAY_BUFFER, vertexData.size_bytes(), vertexData.data(), GL_STATIC_DRAW);

        vertexLayout.Apply();

        _ebo.SetData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), GL_STATIC_DRAW);
        _indicesCount = static_cast<GLsizei>(indices.size());
    }
    GLVertexArray::Unbind();
}

void Mesh::AddSecondaryBuffer(const VertexLayout &layout, std::span<const std::byte> data)
{
    _vao.Bind();
    auto& secondary = _secondaryVbos.emplace_back(GLBuffer{}, layout);
    secondary.buffer.SetData(GL_ARRAY_BUFFER, data.size_bytes(), data.data(), GL_STATIC_DRAW);
    layout.Apply();
    GLVertexArray::Unbind();
}

glm::mat4 Mesh::LocalMatrix() const
{
    // Source-scene transform (e.g. COLLADA Z_UP -> Y_UP baked at the root node by Assimp,
    // plus any intermediate aiNode transforms) is applied on top of the user-controllable
    // PRS transform from Pivot3D.
    return _nodeMatrix * Pivot3D::LocalMatrix();
}
