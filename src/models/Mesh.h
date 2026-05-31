#pragma once

#include <vector>
#include "materials/MaterialBase.h"
#include <memory>
#include <span>

#include "Pivot3D.h"
#include "render/GLBuffer.h"
#include "render/GLVertexArray.h"
#include "render/VertexLayout.h"


class Mesh: public Pivot3D {

	struct SecondaryBuffer {
		GLBuffer buffer;
		VertexLayout layout;
	};

	GLVertexArray _vao;
	GLBuffer _vbo;
	GLBuffer _ebo;
	std::vector<SecondaryBuffer> _secondaryVbos;
	std::shared_ptr<MaterialBase> _material;
	GLsizei _indicesCount = 0;
	// Pre-PRS transform baked from the source scene graph (e.g. Assimp aiNode->mTransformation
	// accumulated from root). Stays identity for assets without scene graph (e.g. BoxModel).
	glm::mat4 _nodeMatrix = glm::mat4(1.0f);
public:
	/*  Functions  */
	// Constructor
    Mesh(const std::shared_ptr<MaterialBase>& mat);
	~Mesh() override = default;
	// Render the mesh
    void Render(const RenderContext &ctx, MaterialBase* material) override;
    const std::shared_ptr<MaterialBase>& GetMaterial() const { return _material; }
	void SetMaterial(const std::shared_ptr<MaterialBase>& material) { _material = material; }
	void SetupMesh(const VertexLayout& vertexLayout, std::span<const std::byte> vertexData, std::span<const GLuint> indices);
	void AddSecondaryBuffer(const VertexLayout& layout, std::span<const std::byte> vertexData);
	void SetNodeMatrix(const glm::mat4& m) { _nodeMatrix = m; }
	const glm::mat4& GetNodeMatrix() const { return _nodeMatrix; }
protected:
	glm::mat4 LocalMatrix() const override;
};