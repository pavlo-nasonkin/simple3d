#pragma once

#include <vector>
#include "Mesh.h"
#include <GL/glew.h>
#include "Model.h"

class Material3D;
class MaterialBase;


class BoxModel: public Model 
{

private:
	
	static std::vector<Vertex> boxVertices;
	static std::vector<GLuint> boxIndices;

	unsigned int _color = 0x888888ff;
public:
	BoxModel();
	~BoxModel() override;
    void Init() override;
    std::shared_ptr<Mesh> processMesh();

	void SetColor(unsigned int color) { _color = color; }
private:
};

