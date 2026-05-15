#ifndef COLORFILTER_H
#define COLORFILTER_H

#include "materials/Filter3d.h"

class ColorFilter: public Filter3D
{
private:
    static std::string _colorFilterCode;
    unsigned int _color;
    std::string _uniformName;
public:
    ColorFilter();
    ~ColorFilter();
    void bind(GLuint program) override;

    unsigned int color() const;
    void setColor(unsigned int color);
};

#endif // COLORFILTER_H
