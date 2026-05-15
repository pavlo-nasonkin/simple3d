#pragma once
#ifndef Texture2D_h__
#define Texture2D_h__

#include <GL/glew.h>
#include <string>
#include <assimp/types.h>

class Texture2D
{
public:
	GLuint id;
    std::string type = "";
    std::string path = "";
public:
	Texture2D();
	~Texture2D();
protected:
	
private:
};

#endif // Texture2D_h__
