#pragma once
#include <map>
#include <string>
#include <string_view>

#include "GL/glew.h"


class UniformsLocationCache final
{
public:
    ~UniformsLocationCache() = default;
    GLint GetUniformLocation(std::string_view name);
    void Reset(GLuint program);

private:
    GLuint _program = 0;
    // std::map + std::less<> даёт heterogeneous lookup по string_view
    // без аллокации временной std::string (C++14).
    std::map<std::string, GLint, std::less<>> _uniformLocations;
};