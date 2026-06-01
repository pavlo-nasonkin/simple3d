#pragma once

#include <string>
#include "GLEWImporter.h"

class Filter3D
{
public:

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

    enum class FilterSlot {
        BaseColor,    // модулирует BASE_COLOR (умножается / заменяется)
        Specular,     // модулирует SPEC_STRENGTH
        Normal,       // переопределяет N (для normal map)
        Emissive,     // добавляется через EMISSIVE
        Overlay,      // постпроцесс над финальным color

        Metallic,
        Roughness,
        AO
    };

    enum ResultType
    {
        VEC3,
        VEC4,
        FLOAT
    };

    Filter3D() = default;
    virtual ~Filter3D() = default;
    virtual void Init();
    virtual void Bind(GLuint program, GLuint firstTextureUnit = 0) {}
    virtual void Unbind(GLuint program, GLuint firstTextureUnit = 0) {}

    const std::string& GetName() const { return _name; }
    void SetName(const std::string& name) { _name = name; }

    const std::string& GetCode() const { return _code; }
    void SetCode(const std::string& code) { _code = code; }

    FilterType GetType() const { return _type; }

    BlendMode GetBlendMode() const { return _blendMode; }
    void SetBlendMode(const BlendMode &blendMode) { _blendMode = blendMode; }

    const std::string& GetGeneratedUniqueName() const { return _generatedUniqueName; }

    FilterSlot GetSlot() const { return _slot; }
    void SetSlot(FilterSlot s) { _slot = s; }

    void SetId(unsigned int id) { _id = id; }

    virtual unsigned int GetUniformsCount() const { return 0; }
    void SetNextUniformId(unsigned int unit) { _nextUniformId = unit; }

    ResultType GetResultType() const { return _resultType; }
    void SetResultType(ResultType type) { _resultType = type; }
protected:

    virtual const std::string& GetBaseFilterCode() const = 0;

    FilterType _type = FilterType::FRAGMENT;
    BlendMode _blendMode = BlendMode::ADD;
    std::string _code;
    std::string _name;
    std::string _generatedUniqueName;
    unsigned int _id = 0;
    FilterSlot _slot = FilterSlot::Overlay;
    unsigned int _nextUniformId = 0;
    ResultType _resultType = ResultType::VEC3;
};