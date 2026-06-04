#pragma once

#include <string>

#include "Filter3d.h"

// Нейтральное описание фильтра для сериализации и инспектора материалов.
// Не зависит от формата хранения (JSON-конвертация — в слое сериализации сцены/префаба).
struct FilterData
{
    std::string type;                                       // "TextureMap" | "NormalMap" | "Color"
    Filter3D::FilterSlot slot = Filter3D::FilterSlot::Overlay;
    Filter3D::BlendMode  blend = Filter3D::BlendMode::NORMAL;

    // Параметры конкретных фильтров (используются те, что релевантны типу):
    std::string texturePath;        // TextureMap / NormalMap — путь к текстуре
    unsigned int color = 0xFFFFFFFF; // Color — упакованный RGBA
};
