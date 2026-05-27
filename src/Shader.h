#pragma once

#include "GLEWImporter.h"
class Shader
{
public:
	Shader() = default;

	explicit Shader(GLuint program) : _program(program) {}
	~Shader();

    Shader(const Shader&) = delete;            //запретить копирование
	Shader& operator=(const Shader&) = delete;
	Shader(Shader&& other) noexcept : _program(other._program) { other._program = 0; }
	Shader& operator=(Shader&& other) noexcept {
		if (this != &other) {
			if (_program) {
				glDeleteProgram(_program);
			}
			_program = other._program;
			other._program = 0;
		}
		return *this;
	}

	GLuint GetProgram() const { return _program; }
	void Use() const;

private:
	GLuint _program = 0;
};