#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include "../GLEWImporter.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


//Assimp
#include <assimp/types.h>

#include "../Shader.h"
#include "../resources/Texture2D.h"
#include "../materials/Material3D.h"
#include "../materials/MaterialBase.h"
#include <memory>
#include "../Pivot3D.h"



enum AttribPointer{
    VERTEX_ID_LOCATION = 0,
    NORMAL_ID_LOCATION = 1,
    UV_ID_LOCATION = 2,
    BONE_ID_LOCATION = 3,
    BONE_WEIGHT_LOCATION = 4
};

struct Vertex {
	// Position
	glm::vec3 Position;
	// Normal
	glm::vec3 Normal;
	// TexCoords
	glm::vec2 TexCoords;
};

#define NUM_BONES_PER_VERTEX 4
struct VertexBoneData
{
    unsigned int IDs[NUM_BONES_PER_VERTEX];
    float Weights[NUM_BONES_PER_VERTEX];
};

class Mesh: public Pivot3D {

    bool _hasBones = false;

    /*  Render data  */
    GLuint vertexAttributesArray{};
    GLuint vertexArrayBuffer{};
    GLuint elementArrayBuffer{};
    GLuint boneArrayBuffer{};

public:
	/*  Mesh Data  */
    std::vector<Vertex> _vertices;
    std::vector<GLuint> _indices;
    std::vector<VertexBoneData> _bones;
    std::shared_ptr<MaterialBase> _material;

	/*  Functions  */
	// Constructor
    Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::shared_ptr<MaterialBase>& mat, const std::vector<VertexBoneData>& bones);
	~Mesh() override = default;
	// Render the mesh
    void render(const RenderContext &ctx, MaterialBase* material) override;
    std::shared_ptr<MaterialBase> material() const;

private:
	void setupMesh();
};