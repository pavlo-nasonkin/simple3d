#ifndef FILTER3D_H
#define FILTER3D_H

#include <string>
#include "GLEWImporter.h"

enum BlendMode
{
    NONE,
    NORMAL,
    ADD,
    MULTIPLY
};

enum FilterType
{
    VERTEX,
    FRAGMENT
};

class Filter3D
{
private:
    static unsigned int _filtersIdCounter;
    BlendMode _blendMode;
protected:
    FilterType _type;
    std::string _code;
    std::string _name;
    std::string _generatedUniqueName;
    unsigned int _id;
public:
    Filter3D();
    ~Filter3D();
    virtual void bind(GLuint program);
    std::string name() const;
    void setName(const std::string& name);
    std::string code() const;
    void setCode(const std::string& code);
    FilterType type() const;
    BlendMode blendMode() const;
    void setBlendMode(const BlendMode &blendMode);
    std::string generatedUniqueName() const;
};


#endif // FILTER3D_H
