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

std::map<std::string, ShaderInfo> ShaderFactory::_vertexShaders;
std::map<std::string, ShaderInfo> ShaderFactory::_fragmentShaders;
std::map<std::string, std::shared_ptr<Shader>> ShaderFactory::_programs;

std::shared_ptr<Shader> ShaderFactory::getShader(std::string vertShaderPath, std::string fragShaderPath)
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

    std::shared_ptr<Shader> shader = std::make_shared<Shader>(Shader(vertex, fragment));
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

	_programs.insert({ key, shader });

	return shader;

}

GLuint ShaderFactory::getFromCacheOrCreate(int type, std::string path)
{
    std::map<std::string, ShaderInfo>* container = nullptr;
	if (type == GL_VERTEX_SHADER) {
		container = &_vertexShaders;
	}
	else if (type == GL_FRAGMENT_SHADER) {
		container = &_fragmentShaders;
	}

	if (container == nullptr) {
		throw "haderFactory::getFromCacheOrCreate invalid shader type";
	}


    GLuint shaderId = 0;

	auto mapIterator = container->find(path);
	if (mapIterator == container->end()) {

        std::string shaderCode = FileUtils::readFile(path);
        const GLchar* shaderCodeCStr = shaderCode.c_str();
        int creationStatus = 0;
        shaderId = createShaderFromSource(type, &shaderCodeCStr, creationStatus);
        if (creationStatus != 0){
            std::cout << "shader creation failed\n" << path << std::endl;
        }
        container->insert({ path, {shaderId, shaderCode} });
	}
	else {
        shaderId = mapIterator->second.shaderId;
	}

    return shaderId;
}



GLuint ShaderFactory::createShaderFromSource(int type, const GLchar** source, int& status)
{
	GLuint shaderId;
	GLint success;
	GLchar infoLog[512];

	// Vertex Shader
	shaderId = glCreateShader(type);
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
