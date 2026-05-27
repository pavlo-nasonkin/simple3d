#include "Shader.h"

#include "Engine.h"
#include "GLUtils.h"

void Shader::Use() const
{
    glUseProgram(_program);
	glCheckError();
}

Shader::~Shader()
{
	if (_program) {
		glDeleteProgram(_program);
	}
}