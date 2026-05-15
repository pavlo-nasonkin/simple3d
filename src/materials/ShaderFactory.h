#pragma once
#ifndef ShaderFactory_h__
#define ShaderFactory_h__

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
private:
    static std::map<std::string, ShaderInfo> _vertexShaders;
    static std::map<std::string, ShaderInfo> _fragmentShaders;
    static std::map<std::string, std::shared_ptr<Shader>> _programs;
public:
    static std::shared_ptr<Shader> getShader(std::string vertShader, std::string fragShader);
	
private:
	static GLuint getFromCacheOrCreate(int type, std::string path);
    static GLuint createShaderFromSource(int type, const GLchar **source, int& status);
};

#endif // ShaderFactory_h__
