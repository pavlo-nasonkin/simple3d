#pragma once
#ifndef FileUtils_h__
#define FileUtils_h__
#include <string>

class FileUtils
{
public:
	static std::string readFile(const std::string& path);
};

#endif // FileUtils_h__