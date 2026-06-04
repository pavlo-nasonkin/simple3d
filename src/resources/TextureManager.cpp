#include "TextureManager.h"

#include <filesystem>
#include <iostream>

#include "Texture2D.h"
#include "Engine.h"
#include "GLUtils.h"

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
}

std::shared_ptr<Texture2D> TextureManager::getTexture(std::string_view texturePath, std::string_view typeName, std::string_view directory)
{
    auto mapIterator = _textures.find(texturePath);
	if (mapIterator != _textures.end()) {
		return mapIterator->second;
	}

    std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();

    texture->id = TextureFromFile(texturePath, directory);
	texture->type = typeName;
	texture->path = texturePath;
	texture->directory = directory;
	auto [inserted, _] = _textures.emplace(std::string(texturePath), std::move(texture));
	return inserted->second;
}

GLuint TextureManager::TextureFromFile(std::string_view filename, std::string_view directory) {
	//Generate texture ID and load texture data 
	std::string fullname;
	if (directory.empty()) {
		// Пустой каталог → filename уже полный путь (напр. при загрузке префаба).
		fullname.assign(filename);
	} else {
		fullname.reserve(directory.size() + 1 + filename.size());
		fullname.append(directory).push_back('/');
		fullname.append(filename);
	}

	// SOIL_FLAG_TEXTURE_REPEATS -> GL_REPEAT wrapping. Without it SOIL defaults to
	// GL_CLAMP_TO_EDGE, which breaks mirrored/tiled UV layouts (e.g. nanosuit): the
	// mirrored half of the mesh has UVs outside [0,1] and gets clamped to the texture
	// border instead of wrapping, producing a smeared/garbage half.
	// NTSC_SAFE_RGB and COMPRESS_TO_DXT are intentionally dropped: the former remaps
	// colors and the latter adds lossy block artifacts, both degrade texture quality.
	GLuint textureID = SOIL_load_OGL_texture
	(
		fullname.c_str(),
		SOIL_LOAD_RGB,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS
	);

	std::stringstream ss;
	ss << "[TextureManager::TextureFromFile] " 	  << "Loaded texture from file: " << fullname
			<< " with result: " << SOIL_last_result();
	Engine::GetInstance().Log(ss.str());

	glCheckError();


	// SOIL 1.0 loading
	// glGenTextures(1, &textureID);
	// int width, height;
	// unsigned char* image = SOIL_load_image(fullname.c_str(), &width, &height, 0, SOIL_LOAD_RGB);

	// auto res = SOIL_last_result();
	// // Assign texture to ID
	// glBindTexture(GL_TEXTURE_2D, textureID);
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// glGenerateMipmap(GL_TEXTURE_2D);

	// // Parameters
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// glBindTexture(GL_TEXTURE_2D, 0);
	// std::free(image);
	return textureID;
}
