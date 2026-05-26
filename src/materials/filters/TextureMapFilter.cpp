#include "TextureMapFilter.h"
#include "utils/StringUtils.h"

std::string TextureMapFilter::_filterCode =
        "uniform sampler2D uSampler{uniform_id};\n"
        "vec4 textureMapFilter{id}()\n"
        "{\n"
        "    return texture(uSampler{uniform_id}, TexCoords);\n"
        "}\n";

TextureMapFilter::TextureMapFilter(const std::shared_ptr<Texture2D>& texture):
    TextureFilterBase(texture)
{
    _name = "textureMapFilter";
    _resultType = ResultType::VEC4;
}