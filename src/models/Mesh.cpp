#include "Mesh.h"
#include "materials/MaterialBase.h"
#include "Pivot3D.h"
#include "GLUtils.h"
#include "utils/Math3d.h"
#include <glm/gtc/matrix_transform.hpp>

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

void Mesh::SetupMesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::vector<VertexBoneData>& bones)
{
    _vao.Bind();
    {
        // Load data into vertex buffers
        _vbo.SetData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        _ebo.SetData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        _indicesCount = static_cast<GLsizei>(indices.size());

        // Set the vertex attribute pointers
        // Vertex Positions
        glEnableVertexAttribArray(VERTEX_ID_LOCATION);
        glVertexAttribPointer(VERTEX_ID_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
        // Vertex Normals
        glEnableVertexAttribArray(NORMAL_ID_LOCATION);
        glVertexAttribPointer(NORMAL_ID_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
        // Vertex Texture Coords
        glEnableVertexAttribArray(UV_ID_LOCATION);
        glVertexAttribPointer(UV_ID_LOCATION, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));

        glEnableVertexAttribArray(TANGENT_ID_LOCATION);
        glVertexAttribPointer(TANGENT_ID_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Tangent));
        if (!bones.empty())
        {
            _bonesVbo.emplace();
            _bonesVbo->SetData(GL_ARRAY_BUFFER, sizeof(VertexBoneData) * bones.size(), bones.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(BONE_ID_LOCATION);
            glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);

            glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);
            glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)offsetof(VertexBoneData, Weights));
        }
        else
        {
            _bonesVbo.reset();
        }
    }
    GLVertexArray::Unbind();
}