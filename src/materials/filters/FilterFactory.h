#pragma once

#include <memory>
#include <string>

struct FilterData;
class Filter3D;

// Создаёт фильтр по его описанию (обратная операция к Filter3D::Serialize).
// Нужна для десериализации материалов и ручного добавления фильтров в инспекторе.
class FilterFactory
{
public:
    // textureDir — каталог для загрузки текстур фильтров (относительно рабочего каталога).
    static std::shared_ptr<Filter3D> Create(const FilterData& data, const std::string& textureDir = "");
};
