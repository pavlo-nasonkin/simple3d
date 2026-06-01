#pragma once

#include <GL/glew.h>
#include <string>

class Texture2D
{
public:
	GLuint id = 0;
    std::string type;
    std::string path;

	Texture2D() = default;
	~Texture2D() = default;
};