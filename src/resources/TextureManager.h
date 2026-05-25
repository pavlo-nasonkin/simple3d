#pragma once

#include <map>
#include <string>
#include <SOIL/SOIL2.h>
#include "resources/Texture2D.h"
#include <memory>
#include <sstream>

class TextureManager
{
    std::map<std::string, std::shared_ptr<Texture2D>, std::less<>> _textures;

public:
	TextureManager();
	~TextureManager();
    std::shared_ptr<Texture2D> getTexture(std::string_view texturePath, std::string_view typeName, std::string_view directory);
    static GLuint TextureFromFile(std::string_view filename, std::string_view directory);
};