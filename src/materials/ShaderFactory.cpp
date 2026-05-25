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

std::map<std::string, ShaderInfo, std::less<>> ShaderFactory::_vertexShaders;
std::map<std::string, ShaderInfo, std::less<>> ShaderFactory::_fragmentShaders;
std::map<std::string, std::shared_ptr<Shader>, std::less<>> ShaderFactory::_programs;

std::shared_ptr<Shader> ShaderFactory::getShader(std::string_view vertShaderPath, std::string_view fragShaderPath)
{
	//search in map
	GLuint vertex = getFromCacheOrCreate(GL_VERTEX_SHADER, vertShaderPath);
	GLuint fragment = getFromCacheOrCreate(GL_FRAGMENT_SHADER, fragShaderPath);

	//find in already created programs
	std::string key = std::to_string(vertex) + "_" + std::to_string(fragment);
	auto mapIterator = _programs.find(key);
	if (mapIterator != _programs.end()) {
		return mapIterator->second;
	}

    std::shared_ptr<Shader> shader = std::make_shared<Shader>(vertex, fragment);
    auto vertexIter = _vertexShaders.find(vertShaderPath);
    if (vertexIter != _vertexShaders.end())
    {
        shader->setVertexSource(vertexIter->second.code);
        shader->setOriginalVertexSource(vertexIter->second.code);
    }

    auto fragIter = _fragmentShaders.find(fragShaderPath);
    if (fragIter != _fragmentShaders.end())
    {
        shader->setFragmentSource(fragIter->second.code);
        shader->setOriginalFragmentSource(fragIter->second.code);
    }

	_programs.emplace(std::move(key), shader);
	return shader;

}

GLuint ShaderFactory::getFromCacheOrCreate(int type, std::string_view path)
{
	auto* container = (type == GL_VERTEX_SHADER) ? &_vertexShaders
					: (type == GL_FRAGMENT_SHADER) ? &_fragmentShaders
					: nullptr;
	if (!container) {
		throw std::runtime_error("ShaderFactory::getFromCacheOrCreate invalid shader type");
	}
	if (auto it = container->find(path); it != container->end()) {
		return it->second.shaderId;
	}

	std::string pathStr(path);
	std::string code = FileUtils::readFile(pathStr);
	const GLchar* src = code.c_str();
	int status = 0;
	GLuint id = createShaderFromSource(type, &src, status);
	if (status != 0) {
		std::cout << "shader creation failed\n" << pathStr << std::endl;
	}
	container->emplace(std::move(pathStr), ShaderInfo{id, std::move(code)});
	return id;
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
