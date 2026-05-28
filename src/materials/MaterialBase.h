#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include "ShaderFactory.h"
#include "render/RenderContext.h"
#include "GL/glew.h"
#include "lighting/ILightingModel.h"
#include "render/UniformsLocationCache.h"


class Shader;
class Mesh;

enum CullFaceMode
{
    none,
    back,
    front
};

class MaterialBase
{
public:
    static const std::string MATERIAL_UPDATE_EVENT;
protected:
    static unsigned int _idCounter;
    std::string _name;
    unsigned int _id;

    std::shared_ptr<Shader> _shader;
    std::string _vertexShaderPath;
    std::string _fragmentShaderPath;
    CullFaceMode _cullFace;
    UniformsLocationCache _uniformCache;
public:
    MaterialBase(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
	virtual ~MaterialBase() = default;
    virtual void Build();
	virtual void Bind(const RenderContext& ctx, const Mesh* mesh = nullptr);
	virtual void Unbind();
    const std::string& GetName() const { return _name; }
    void SetName(const std::string &name) { _name = name; }

    unsigned int GetId() const { return _id; }
    void SetId(unsigned int id) { _id = id; }

    const std::shared_ptr<Shader>& GetShader() const { return _shader; }
    void SetShader(const std::shared_ptr<Shader> &shader) { _shader = shader; }

    void SetCullFace(const CullFaceMode& cullFace) { _cullFace = cullFace; }
    CullFaceMode GetCullFace() const { return _cullFace; }
    static void ClearProgramCache();
protected:
    virtual const ShaderFactory::CompiledShader& BuildVertexShader() const;
    virtual const ShaderFactory::CompiledShader& BuildFragmentShader() const;

    static size_t HashSources(const std::string &vertexSource, const std::string &fragmentSource);
    static GLuint LinkProgram(GLuint vShader, GLuint fShader);
    static std::unordered_map<size_t, std::shared_ptr<Shader>> _programCache;
};