#include "StringUtils.h"

void StringUtils::replace(std::string& source, const std::string& what, const std::string& with)
{
    size_t index = 0;
    while (true) {
         /* Locate the substring to replace. */
         index = source.find(what, index);
         if (index == std::string::npos) break;

         /* Make the replacement. */
         source.replace(index, what.size(), with);

         /* Advance index forward so the next iteration doesn't pick it up as well. */
         index += with.size();
    }
}

void StringUtils::replaceLast(std::string &source, const std::string &what, const std::string &with)
{
    size_t index = 0;
    bool found = false;
    while (true)
    {
        /* Locate the substring to replace. */
        size_t curIndex = source.find(what, index);

        if (index == std::string::npos) break;
        found = true;
        index = curIndex;
        index += with.size();
    }

    if (found)
    {
        source.replace(index, what.size(), with);
    }
}
