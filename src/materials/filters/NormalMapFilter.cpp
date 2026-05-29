#include "NormalMapFilter.h"

#include "TextureFilterBase.h"
#include "utils/StringUtils.h"

std::string NormalMapFilter::_filterCode =
        "uniform sampler2D uSampler{uniform_id};\n"
        "vec3 normalMapFilter{id}()\n"
        "{\n"
        "    vec3 ts = texture(uSampler{uniform_id}, TexCoords0).rgb * 2.0 - 1.0;\n"
        "    return normalize(TBN * ts);\n"
        "}\n";

NormalMapFilter::NormalMapFilter(const std::shared_ptr<Texture2D>& texture):
    TextureFilterBase(texture)
{
    _name = "normalMapFilter";
    _resultType = ResultType::VEC3;
}