#include "Shader.h"

#include "Engine.h"
#include "GLUtils.h"

void Shader::setOriginalVertexSource(const std::string &originalVertexSource)
{
    _originalVertexSource = originalVertexSource;
}

void Shader::setOriginalFragmentSource(const std::string &originalFragmentSource)
{
    _originalFragmentSource = originalFragmentSource;
}

Shader::Shader(const GLchar * vertexPath, const GLchar * fragmentPath)
    :_inited(false)
{
    // 1. Retrieve the vertex/fragment source code from filePath
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;
	// ensures ifstream objects can throw exceptions:
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		// Open files
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream, fShaderStream;
		// Read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file handlers
		vShaderFile.close();
		fShaderFile.close();
		// Convert stream into GLchar array
        _vertexSource = vShaderStream.str();
        _fragmentSource = fShaderStream.str();
        _originalVertexSource = _vertexSource;
        _originalFragmentSource = _fragmentSource;
	}
	catch (const std::ifstream::failure& e)
	{
	    Engine::GetInstance().Log("Shader file not successfully read: " + std::string(e.what()) + " vertexPath: " + vertexPath + " fragmentPath: " + fragmentPath);
	}

    build();
}

Shader::Shader(GLuint vertexShaderId, GLuint fragmentShaderId)
    :_inited(false)
{
	GLint success;
	GLchar infoLog[512];
	// Shader Program
	this->Program = glCreateProgram();
	glAttachShader(this->Program, vertexShaderId);
	glAttachShader(this->Program, fragmentShaderId);
	glLinkProgram(this->Program);
	// Print linking errors if any
	glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
    if (success)
	{
        _inited = true;
    }
    else
    {
        glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }


}

Shader::Shader()
    :_inited(false)
{

}

void Shader::dispose()
{
    glUseProgram(0);
    glDeleteProgram(this->Program);

}

void Shader::build()
{
    const GLchar* vShaderCode = _vertexSource.c_str();
    const GLchar* fShaderCode = _fragmentSource.c_str();


    // 2. Compile shaders
    GLuint vertex, fragment;
    GLint success;
    GLchar infoLog[512];

    bool vertexShaderCreated = false;
    bool fragmentShaderCreated = false;
    bool programLinked = false;

    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    // Print compile errors if any
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (success)
    {
        vertexShaderCreated = true;
    }
    else
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Similiar for Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    // Print compile errors if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (success)
    {
        fragmentShaderCreated = true;
    }
    else
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }


    // Shader Program
    this->Program = glCreateProgram();
    glAttachShader(this->Program, vertex);
    glAttachShader(this->Program, fragment);
    glLinkProgram(this->Program);
    // Print linking errors if any
    glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
    if (success)
    {
        programLinked = true;
    }
    else
    {
        glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
//        std::cout << vertexPath << "asdasd" << std::endl;
//        std::cout << fragmentPath << std::endl;
    }
    _inited = programLinked;
    // Delete the shaders as they're linked into our program now and no longer necessery
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use()
{
	glUseProgram(this->Program);
	glCheckError();
}

std::string Shader::vertexSource() const
{
    return _vertexSource;
}

std::string Shader::fragmentSource() const
{
    return _fragmentSource;
}

void Shader::setVertexSource(const std::string& vertexSource)
{
    _vertexSource = vertexSource;
}

void Shader::setFragmentSource(const std::string& fragmentSource)
{
    _fragmentSource = fragmentSource;
}

std::string Shader::originalVertexSource() const
{
    return _originalVertexSource;
}

std::string Shader::originalFragmentSource() const
{
    return _originalFragmentSource;
}
