#include "ShaderFactory.h"
#include "Shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "utils/FileUtils.h"
#include "GLEWImporter.h"
#include <algorithm>
#include <memory>
#include <ranges>
std::map<std::string, std::string, std::less<>> ShaderFactory::_shaderSources;
std::map<std::string, ShaderFactory::CompiledShader, std::less<>> ShaderFactory::_vertexShadersByPath;
std::map<std::string, ShaderFactory::CompiledShader, std::less<>> ShaderFactory::_fragmentShadersByPath;

std::map<std::size_t, ShaderFactory::CompiledShader> ShaderFactory::_vertexShadersByHash;
std::map<std::size_t, ShaderFactory::CompiledShader> ShaderFactory::_fragmentShadersByHash;


const ShaderFactory::CompiledShader& ShaderFactory::GetCompiledShader(GLenum type, std::string_view path)
{
	auto* container = (type == GL_VERTEX_SHADER) ? &_vertexShadersByPath
					: (type == GL_FRAGMENT_SHADER) ? &_fragmentShadersByPath
					: nullptr;
	if (!container) {
		throw std::runtime_error("ShaderFactory::GetCompiledShader invalid shader type");
	}
	if (auto it = container->find(path); it != container->end()) {
		return it->second;
	}

	std::string pathStr(path);
	std::string code = GetShaderSource(path);
	const GLchar* src = code.c_str();
	int status = 0;
	GLuint id = createShaderFromSource(type, &src, status);
	if (status != 0) {
		std::cout << "shader creation failed\n" << pathStr << std::endl;
	}

	auto [it, _] = container->emplace(std::move(pathStr), CompiledShader{id, std::move(code)});
	return it->second;
}

const ShaderFactory::CompiledShader& ShaderFactory::GetCompiledShaderFromSource(GLenum type, const std::string &source) {
	auto* container = (type == GL_VERTEX_SHADER) ? &_vertexShadersByHash
					: (type == GL_FRAGMENT_SHADER) ? &_fragmentShadersByHash
					: nullptr;
	if (!container) {
		throw std::runtime_error("ShaderFactory::GetCompiledShaderFromSource invalid shader type");
	}
	std::size_t hash = std::hash<std::string>{}(source);
	if (auto it = container->find(hash); it != container->end()) {
		return it->second;
	}

	const GLchar* src = source.c_str();
	int status = 0;
	GLuint id = createShaderFromSource(type, &src, status);
	if (status != 0) {
		std::cout << "shader creation from source failed" << std::endl;
	}

	auto [it, _] = container->emplace(hash, CompiledShader{id, source});
	return it->second;
}

const std::string& ShaderFactory::GetShaderSource(std::string_view path) {
	if (auto it = _shaderSources.find(path); it != _shaderSources.end()) {
		return it->second;
	}

	std::string pathStr(path);
	std::string code = FileUtils::readFile(pathStr);

	auto [it, _] = _shaderSources.emplace(std::move(pathStr), std::move(code));
	return it->second;
}

void ShaderFactory::Cleanup() {
	for (const auto &shader: _vertexShadersByPath | std::views::values) {
		glDeleteShader(shader.id);
	}
	for (const auto &shader: _fragmentShadersByPath | std::views::values) {
		glDeleteShader(shader.id);
	}
	for (const auto &shader: _vertexShadersByHash | std::views::values) {
		glDeleteShader(shader.id);
	}
	for (const auto &shader: _fragmentShadersByHash | std::views::values) {
		glDeleteShader(shader.id);
	}
	_shaderSources.clear();
	_vertexShadersByPath.clear();
	_fragmentShadersByPath.clear();
	_vertexShadersByHash.clear();
	_fragmentShadersByHash.clear();
}

GLuint ShaderFactory::createShaderFromSource(int type, const GLchar** source, int& status)
{
	GLint success;
	GLchar infoLog[512];

	// Vertex Shader
	GLuint shaderId = glCreateShader(type);
	glShaderSource(shaderId, 1, source, NULL);
	glCompileShader(shaderId);
	// Print compile errors if any
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        status = 1;
	};
    return shaderId;
}
