#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include "ShaderFactory.h"
#include "render/RenderContext.h"
#include "GL/glew.h"

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
protected:
    std::shared_ptr<Shader> _shader;
    std::string _vertexShaderPath;
    std::string _fragmentShaderPath;
    CullFaceMode _cullFace;
public:
    MaterialBase(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
	virtual ~MaterialBase() = default;
    virtual void Build();
	virtual void Bind(const RenderContext& ctx, const Mesh* mesh = nullptr);
	virtual void Unbind();
    std::string name() const;
    void setName(const std::string &name);

    unsigned int id() const;
    void setId(unsigned int id);

    std::shared_ptr<Shader> shader() const;
    void setShader(const std::shared_ptr<Shader> &shader);

    void setCullFace(const CullFaceMode& cullFace);
    CullFaceMode cullFace() const;
    virtual std::shared_ptr<MaterialBase> Clone() const;

    static void ClearProgramCache();
protected:
    virtual const ShaderFactory::CompiledShader& BuildVertexShader() const;
    virtual const ShaderFactory::CompiledShader& BuildFragmentShader() const;
    static size_t HashSources(const std::string &vertexSource, const std::string &fragmentSource);
    static GLuint LinkProgram(GLuint vShader, GLuint fShader);
    static std::unordered_map<size_t, std::shared_ptr<Shader>> _programCache;
};