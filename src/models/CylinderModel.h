#pragma once

#include "PrimitiveModel.h"

class CylinderModel: public PrimitiveModel
{
protected:
	const char* GeometryKey() const override { return "primitive:cylinder"; }
	void GenerateGeometry(std::vector<VertexTypes::Vertex>& vertices,
	                      std::vector<GLuint>& indices) const override;
};
