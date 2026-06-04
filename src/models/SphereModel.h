#pragma once

#include "PrimitiveModel.h"

class SphereModel: public PrimitiveModel
{
protected:
	const char* GeometryKey() const override { return "primitive:sphere"; }
	void GenerateGeometry(std::vector<VertexTypes::Vertex>& vertices,
	                      std::vector<GLuint>& indices) const override;
};
