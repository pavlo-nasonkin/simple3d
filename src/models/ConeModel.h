#pragma once

#include "PrimitiveModel.h"

class ConeModel: public PrimitiveModel
{
protected:
	const char* GeometryKey() const override { return "primitive:cone"; }
	void GenerateGeometry(std::vector<VertexTypes::Vertex>& vertices,
	                      std::vector<GLuint>& indices) const override;
};
