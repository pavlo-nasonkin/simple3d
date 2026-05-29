#pragma once

#include <vector>
#include "Mesh.h"
#include <GL/glew.h>
#include "Model.h"
#include "render/VertexTypes.h"

class ColorFilter;
class Material3D;
class MaterialBase;


class BoxModel: public Model 
{

private:
	
	static std::vector<VertexTypes::Vertex> boxVertices;
	static std::vector<GLuint> boxIndices;

	unsigned int _color = 0x000000FF;
	std::shared_ptr<ColorFilter> _colorFilter;
public:
	BoxModel();
	~BoxModel() override = default;
    void Init() override;
    std::shared_ptr<Mesh> ProcessMesh();

	void SetColor(unsigned int color);

private:
};

