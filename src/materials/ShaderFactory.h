#pragma once

#include <string>
#include <map>
#include "GLEWImporter.h"
#include <memory>

class Shader;
struct ShaderInfo
{
    GLuint shaderId;
    std::string code;
};

class ShaderFactory
{
    static std::map<std::string, ShaderInfo, std::less<>> _vertexShaders;
    static std::map<std::string, ShaderInfo, std::less<>> _fragmentShaders;
    static std::map<std::string, std::shared_ptr<Shader>, std::less<>> _programs;
public:
    static std::shared_ptr<Shader> getShader(std::string_view vertShader, std::string_view fragShader);
	
private:
	static GLuint getFromCacheOrCreate(int type, std::string_view path);
    static GLuint createShaderFromSource(int type, const GLchar **source, int& status);
};