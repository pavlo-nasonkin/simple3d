#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "GLEWImporter.h"
class Shader
{
private:
    std::string _vertexSource;
    std::string _fragmentSource;
    std::string _originalVertexSource;
    std::string _originalFragmentSource;
    bool _inited;
public:
	// The program ID
	GLuint Program;
	// Constructor reads and builds the shader
	Shader(const GLchar* vertexPath, const GLchar* fragmentPath);
	Shader(GLuint vertexShaderId, GLuint fragmentShaderId);
    Shader();
    void dispose();
    void build();
	// Use the program
    void use();
    std::string vertexSource() const;
    std::string fragmentSource() const;
    void setVertexSource(const std::string& vertexSource);
    void setFragmentSource(const std::string& fragmentSource);
    std::string originalVertexSource() const;
    std::string originalFragmentSource() const;
    void setOriginalVertexSource(const std::string &originalVertexSource);
    void setOriginalFragmentSource(const std::string &originalFragmentSource);
};

#endif
