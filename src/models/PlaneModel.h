#pragma once

#include <vector>
#include <string>
#include <memory>
#include <GL/glew.h>
#include "Model.h"
#include "render/VertexTypes.h"

class ColorFilter;
class Material3D;
class MaterialBase;

// Единичный горизонтальный quad в плоскости XZ (нормаль +Y), центр в начале координат.
class PlaneModel: public Model
{
private:
	static std::vector<VertexTypes::Vertex> planeVertices;
	static std::vector<GLuint> planeIndices;

	unsigned int _color = 0xFFFFFFFF;
	std::shared_ptr<ColorFilter> _colorFilter;

	std::string _materialDir;   // если задан — грузится PBR-материал из папки по именам Megascans
	float _tiling = 1.0f;       // во сколько раз повторять текстуру по UV
	float _roughnessScale = 1.0f; // множитель к roughness (меньше 1 = глянцевее)
public:
	PlaneModel();
	~PlaneModel() override = default;
	void Init() override;
	std::shared_ptr<Pivot3D> ProcessMesh();

	void SetColor(unsigned int color);

	// Папка с PBR-картами (BaseColor/Normal/Roughness/AO/Metalness) по соглашению Megascans.
	// Вызывать ДО Init(). Если задана — color-фильтр не используется, ставится PBR-материал.
	void SetMaterialDirectory(const std::string& directory) { _materialDir = directory; }
	void SetTiling(float tiling) { _tiling = tiling; }
	// Множитель roughness (<1 — глянцевее/ярче блик, >1 — матовее). Вызывать до Init().
	void SetRoughnessScale(float scale) { _roughnessScale = scale; }
};
