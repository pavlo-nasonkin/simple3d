#pragma once

#include <vector>
#include <memory>
#include <GL/glew.h>

#include "Model.h"
#include "render/VertexTypes.h"

class ColorFilter;
class Mesh;

// База для процедурных примитивов (Sphere/Cylinder/Cone/Torus): общий материал
// (Material3D + ColorFilter), сборка меша и шаринг геометрии через GeometryRegistry.
// Подкласс задаёт ключ кэша и генерацию вершин/индексов.
class PrimitiveModel: public Model
{
protected:
	unsigned int _color = 0xCCCCCCFF;
	std::shared_ptr<ColorFilter> _colorFilter;

	virtual const char* GeometryKey() const = 0;
	virtual void GenerateGeometry(std::vector<VertexTypes::Vertex>& vertices,
	                              std::vector<GLuint>& indices) const = 0;

public:
	~PrimitiveModel() override = default;
	void Init() override;
	void SetColor(unsigned int color);
	std::shared_ptr<Mesh> ProcessMesh();
};
