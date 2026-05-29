#pragma once

#include <vector>
#include "GLEWImporter.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


//Assimp
#include <assimp/types.h>

#include "resources/Texture2D.h"
#include "materials/MaterialBase.h"
#include <memory>
#include <optional>

#include "Pivot3D.h"
#include "render/GLBuffer.h"
#include "render/GLVertexArray.h"


enum AttribPointer{
    VERTEX_ID_LOCATION = 0,
    NORMAL_ID_LOCATION = 1,
    UV_ID_LOCATION = 2,
	TANGENT_ID_LOCATION = 3,
    BONE_ID_LOCATION = 4,
    BONE_WEIGHT_LOCATION = 5
};

struct Vertex {
	// Position
	glm::vec3 Position;
	// Normal
	glm::vec3 Normal;
	// TexCoords
	glm::vec2 TexCoords;
	glm::vec3 Tangent;
};

#define NUM_BONES_PER_VERTEX 4
struct VertexBoneData
{
    unsigned int IDs[NUM_BONES_PER_VERTEX];
    float Weights[NUM_BONES_PER_VERTEX];
};

class Mesh: public Pivot3D {

	GLVertexArray _vao;
	GLBuffer _vbo;
	GLBuffer _ebo;
	std::optional<GLBuffer> _bonesVbo;
	std::shared_ptr<MaterialBase> _material;
	GLsizei _indicesCount = 0;
public:
	/*  Functions  */
	// Constructor
    Mesh(const std::shared_ptr<MaterialBase>& mat);
	~Mesh() override = default;
	// Render the mesh
    void Render(const RenderContext &ctx, MaterialBase* material) override;
    const std::shared_ptr<MaterialBase>& GetMaterial() const { return _material; }
	void SetMaterial(const std::shared_ptr<MaterialBase>& material) { _material = material; }
	void SetupMesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::vector<VertexBoneData>& bones);
};