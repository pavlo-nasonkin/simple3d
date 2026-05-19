#pragma once

#ifndef TextureManager_h__
#define TextureManager_h__
#include <cstdlib>
#include <map>
#include <string>
#include "GLEWImporter.h"
#include <SOIL/SOIL2.h>
#include <assimp/types.h>
#include "resources/Texture2D.h"
#include <memory>
#include <sstream>

#include "Engine.h"

class TextureManager
{
private:
    std::map<std::string, std::shared_ptr<Texture2D>> _textures;

public:
	TextureManager();
	~TextureManager();
    std::shared_ptr<Texture2D> getTexture(const std::string& texturePath, const std::string& typeName, const std::string& directory);
    static GLuint TextureFromFile(const std::string& filename, const std::string& directory)
	{
		//Generate texture ID and load texture data 
        std::string fullname = directory + '/' + filename;
    	GLuint textureID = SOIL_load_OGL_texture
		(
			fullname.c_str(),
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
		);;

    	std::stringstream ss;
    	ss << "[TextureManager::TextureFromFile] " 	  << "Loaded texture from file: " << fullname
    	<< " with result: " << SOIL_last_result();
    	Engine::Log(ss.str());



		// glGenTextures(1, &textureID);
		// int width, height;
  //       unsigned char* image = SOIL_load_image(fullname.c_str(), &width, &height, 0, SOIL_LOAD_RGB);
  //

  //
  //   	auto res = SOIL_last_result();
		// // Assign texture to ID
		// glBindTexture(GL_TEXTURE_2D, textureID);
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// glGenerateMipmap(GL_TEXTURE_2D);
  //
		// // Parameters
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// glBindTexture(GL_TEXTURE_2D, 0);
		// std::free(image);
		return textureID;
	}
protected:
	
private:
};

#endif // TextureManager_h__
