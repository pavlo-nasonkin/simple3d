#include "FilterFactory.h"

#include "FilterData.h"
#include "Filter3d.h"
#include "TextureMapFilter.h"
#include "NormalMapFilter.h"
#include "ColorFilter.h"

#include "Engine.h"
#include "resources/Texture2D.h"
#include "resources/TextureManager.h"

std::shared_ptr<Filter3D> FilterFactory::Create(const FilterData& data, const std::string& textureDir)
{
    std::shared_ptr<Filter3D> filter;

    if (data.type == "TextureMap" || data.type == "NormalMap") {
        if (data.texturePath.empty()) {
            return nullptr; // текстурный фильтр без текстуры — невалиден
        }
        // texturePath обычно полный (см. TextureFilterBase::Serialize) → каталог пустой,
        // TextureManager воспримет его как полный путь. textureDir — fallback.
        std::string path = data.texturePath;
        std::string dir = textureDir;
        if (path.find('/') != std::string::npos) {
            dir = "";
        }
        auto texture = Engine::GetInstance().GetTextureManager()->getTexture(
            path, "texture", dir);

        if (data.type == "NormalMap") {
            filter = std::make_shared<NormalMapFilter>(texture);
        } else {
            filter = std::make_shared<TextureMapFilter>(texture);
        }
    } else if (data.type == "Color") {
        auto color = std::make_shared<ColorFilter>();
        color->SetColor(data.color);
        filter = color;
    }

    if (filter) {
        filter->SetSlot(data.slot);
        filter->SetBlendMode(data.blend);
    }
    return filter;
}
