#pragma once

#include "PrimitiveModel.h"

class TorusModel: public PrimitiveModel
{
protected:
	const char* GeometryKey() const override { return "primitive:torus"; }
	void GenerateGeometry(std::vector<VertexTypes::Vertex>& vertices,
	                      std::vector<GLuint>& indices) const override;
};
