#pragma once

// Панель «Materials Inspector»: материал выбранного меша — lighting, roughnessScale
// и список фильтров (слот/блендинг/параметры). Изменение слота/блендинга требует
// регенерации шейдера (Material3D::Build).
class MaterialsInspectorPanel
{
public:
    void Draw(bool* open);
};
