#pragma once

#include <GL/glew.h>
#include <string>

class Texture2D
{
public:
	GLuint id = 0;
    std::string type;
    std::string path;        // имя файла, как передано в TextureManager
    std::string directory;   // каталог (для сборки полного пути при сериализации)

	Texture2D() = default;
	~Texture2D() = default;
};