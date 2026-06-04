#pragma once

#include "Filter3d.h"

class ColorFilter: public Filter3D
{

public:
    ColorFilter();
    ~ColorFilter() override = default;
    void Init() override;
    void Bind(GLuint program, GLuint firstTextureUnit = 0) override;

    unsigned int GetColor() const { return _color; }
    void SetColor(unsigned int color) { _color = color; }
    unsigned int GetUniformsCount() const override { return 1; }

    std::string GetTypeName() const override { return "Color"; }
    FilterData Serialize() const override; // + color
protected:
    const std::string& GetBaseFilterCode() const override {
        return _colorFilterCode;
    }

private:
    static std::string _colorFilterCode;
    unsigned int _color;
    std::string _uniformName;
};