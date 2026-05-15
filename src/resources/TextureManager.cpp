#include "TextureManager.h"
#include "Texture2D.h"

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
}

std::shared_ptr<Texture2D> TextureManager::loadTexture(std::string /*name*/)
{
	return nullptr;
}

std::shared_ptr<Texture2D> TextureManager::getTexture(std::string texturePath, std::string typeName, std::string directory)
{
    auto mapIterator = _textures.find(texturePath);
	if (mapIterator != _textures.end()) {
		return mapIterator->second;
	}

    std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>(Texture2D());

    texture->id = TextureManager::TextureFromFile(texturePath, directory);
	texture->type = typeName;
	texture->path = texturePath;
    _textures.insert(std::pair<std::string, std::shared_ptr<Texture2D>>(texturePath, texture));
	return texture;
}
