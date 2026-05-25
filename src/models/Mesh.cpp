#include "Mesh.h"
#include "materials/MaterialBase.h"
#include "Pivot3D.h"
#include "GLUtils.h"
#include "utils/Math3d.h"
#include <glm/gtc/matrix_transform.hpp>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::shared_ptr<MaterialBase>& mat, const std::vector<VertexBoneData>& bones)
    :Pivot3D(),
    _vertices(vertices),
    _indices(indices),
    _bones(bones),
    _material(mat)
{
    _hasBones = !bones.empty();
    _name = "Mesh";
	// Now that we have all the required data, set the vertex buffers and its attribute pointers.
	setupMesh();
}

// Render the mesh

void Mesh::Render(const RenderContext &ctx, MaterialBase* material)
{
    RenderContext context =  ctx;
    context.model = ctx.model * LocalMatrix();
    if (material == nullptr) {
        material = _material.get();
    }

    material->bind(context, this);
    glBindVertexArray(vertexAttributesArray);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    material->unbind();

    for (auto& child : _children) {
        child->Render(context, material);
    }
}

/*  Functions    */
// Initializes all the buffer objects/arrays

void Mesh::setupMesh()
{
    // Create buffers/arrays
    glGenVertexArrays(1, &vertexAttributesArray);
    glGenBuffers(1, &vertexArrayBuffer);
    glGenBuffers(1, &elementArrayBuffer);

    if (_hasBones)
    {
        glGenBuffers(1, &boneArrayBuffer);
        glCheckError();
    }

    glBindVertexArray(vertexAttributesArray);
    {
        // Load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, vertexArrayBuffer);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(Vertex), &_vertices[0], GL_STATIC_DRAW);



        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(GLuint), &_indices[0], GL_STATIC_DRAW);


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

        if (_hasBones)
        {
            glBindBuffer(GL_ARRAY_BUFFER, boneArrayBuffer);

            glBufferData(GL_ARRAY_BUFFER, sizeof(_bones[0]) * _bones.size(), &_bones[0], GL_STATIC_DRAW);
            glEnableVertexAttribArray(BONE_ID_LOCATION);
            glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);

            glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);
            glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)16);
            std::cout << "BindBones" << std::endl;
        }

    }
    glBindVertexArray(0);
}

std::shared_ptr<MaterialBase> Mesh::material() const
{
    return _material;
}
