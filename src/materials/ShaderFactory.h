#pragma once

#include <string>
#include <map>
#include "GLEWImporter.h"
#include <memory>

class Shader;

class ShaderFactory
{
public:
    struct CompiledShader {
        GLuint id = 0;
        std::string source;
    };

    // Парсит файл (один раз) + компилирует shader-object (один раз).
    // Возвращает GLuint, готовый к линковке.
    static const CompiledShader& GetCompiledShader(GLenum type, std::string_view path);
    static const CompiledShader& GetCompiledShaderFromSource(GLenum type, const std::string& source);
    // Отдаёт оригинальный текст исходника (нужен Material'у для подстановки фильтров).
    static const std::string& GetShaderSource(std::string_view path);
    static void Cleanup();
	
private:

    static std::map<std::string, std::string, std::less<>> _shaderSources;
    static std::map<std::string, CompiledShader, std::less<>> _vertexShadersByPath;
    static std::map<std::string, CompiledShader, std::less<>> _fragmentShadersByPath;

    static std::map<std::size_t, CompiledShader> _vertexShadersByHash;
    static std::map<std::size_t, CompiledShader> _fragmentShadersByHash;

    static GLuint createShaderFromSource(int type, const GLchar **source, int& status);
};