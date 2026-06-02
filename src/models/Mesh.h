#pragma once

#include <memory>

#include "Pivot3D.h"

class MaterialBase;
class Geometry;

// Лёгкий узел сцены: ссылается на (шарящуюся) Geometry и материал, добавляет
// transform/иерархию от Pivot3D. GL-ресурсами больше не владеет — это Geometry.
class Mesh: public Pivot3D {

	std::shared_ptr<Geometry> _geometry;
	std::shared_ptr<MaterialBase> _material;
	// Pre-PRS transform из исходного scene graph (напр. накопленный aiNode->mTransformation).
	// Для ассетов без графа (BoxModel/PlaneModel) остаётся единичной.
	glm::mat4 _nodeMatrix = glm::mat4(1.0f);
public:
	Mesh(std::shared_ptr<Geometry> geometry, std::shared_ptr<MaterialBase> material);
	~Mesh() override = default;

    void Render(const RenderContext &ctx, MaterialBase* material) override;

    const std::shared_ptr<MaterialBase>& GetMaterial() const { return _material; }
	void SetMaterial(const std::shared_ptr<MaterialBase>& material) { _material = material; }
	const std::shared_ptr<Geometry>& GetGeometry() const { return _geometry; }

	void SetNodeMatrix(const glm::mat4& m) { _nodeMatrix = m; }
	const glm::mat4& GetNodeMatrix() const { return _nodeMatrix; }
protected:
	glm::mat4 LocalMatrix() const override;
};
