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
class Pivot3D;

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
    std::string _compiledVertexSource;
    std::string _compiledFragmentSource;
public:
    MaterialBase(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
	virtual ~MaterialBase() = default;
    virtual void Build();
	virtual void Bind(const RenderContext& ctx, const Pivot3D* node = nullptr);
	virtual void Unbind();
    const std::string& GetName() const { return _name; }
    void SetName(const std::string &name) { _name = name; }

    unsigned int GetId() const { return _id; }
    void SetId(unsigned int id) { _id = id; }

    const std::shared_ptr<Shader>& GetShader() const { return _shader; }
    void SetShader(const std::shared_ptr<Shader> &shader) { _shader = shader; }

    void SetCullFace(const CullFaceMode& cullFace) { _cullFace = cullFace; }
    CullFaceMode GetCullFace() const { return _cullFace; }

    const std::string& GetVertexShaderPath() const { return _vertexShaderPath; }
    const std::string& GetFragmentShaderPath() const { return _fragmentShaderPath; }

    // Финальный (сгенерированный) код шейдеров — доступен после Build/BuildFromSources.
    const std::string& GetCompiledVertexSource() const { return _compiledVertexSource; }
    const std::string& GetCompiledFragmentSource() const { return _compiledFragmentSource; }

    static void ClearProgramCache();
protected:
    virtual const ShaderFactory::CompiledShader& BuildVertexShader() const;
    virtual const ShaderFactory::CompiledShader& BuildFragmentShader() const;

    // Компиляция готового кода без кодогенерации (compiled-путь, 8.5b).
    void BuildFromSources(const std::string& vertexSource, const std::string& fragmentSource);

    static size_t HashSources(const std::string &vertexSource, const std::string &fragmentSource);
    static GLuint LinkProgram(GLuint vShader, GLuint fShader);
    static std::unordered_map<size_t, std::shared_ptr<Shader>> _programCache;
};