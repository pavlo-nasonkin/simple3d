#ifndef STRINGUTILS_H
#define STRINGUTILS_H
#include <string>

class StringUtils
{
public:
    static void replace(std::string& source, const std::string& what, const std::string& with);
    static void replaceLast(std::string& source, const std::string& what, const std::string& with);
};

#endif // STRINGUTILS_H
