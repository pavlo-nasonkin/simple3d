#include "FileUtils.h"

#include <fstream>
#include <sstream>
#include <iostream>
//#include <windows.h>


std::string FileUtils::readFile(std::string path)
{
//	TCHAR result[MAX_PATH];
//	auto dir = GetModuleFileName(NULL, result, MAX_PATH);


	std::string vertexCode;
	std::ifstream fileStream;
	// ensures ifstream objects can throw exceptions:
	fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		// Open files
		fileStream.open(path.c_str());
		std::stringstream stringStream;
		// Read file's buffer contents into streams
		stringStream << fileStream.rdbuf();
		// close file handlers
		fileStream.close();
		// Convert stream into GLchar array
		return stringStream.str();
	}
	catch (std::ifstream::failure& e)
	{
		std::cout << "ERROR::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		std::cout << path << std::endl;
	}
	return "";
}

