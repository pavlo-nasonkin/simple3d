

#include "UniformsLocationCache.h"

GLint UniformsLocationCache::GetUniformLocation(std::string_view name)
{
    // find(string_view) работает без аллокации благодаря std::less<>.
    if (auto it = _uniformLocations.find(name); it != _uniformLocations.end()) {
        return it->second;
    }
    // На промахе кэша всё равно нужен null-terminated c_str для GL.
    // string_view::data() этого не гарантирует — собираем std::string один раз.
    std::string nameStr(name);
    GLint loc = glGetUniformLocation(_program, nameStr.c_str());
    _uniformLocations.emplace(std::move(nameStr), loc);
    return loc;
}

void UniformsLocationCache::Reset(GLuint program) {
    _program = program;
    _uniformLocations.clear();
}
