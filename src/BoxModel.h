#pragma once

#include <vector>

#include "Mesh.h"
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <string>
#include <assimp/scene.h>
#include "resources/Texture2D.h"
#include "Model.h"

class Material3D;
class MaterialBase;


class BoxModel: public Model 
{

private:
	
	static std::vector<Vertex> boxVertices;
	static std::vector<GLuint> boxIndices;
public:
	BoxModel();
	~BoxModel() override;
    void init() override;
    std::shared_ptr<Mesh> processMesh();
private:
};

