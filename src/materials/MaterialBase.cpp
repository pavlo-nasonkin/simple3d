#include "MaterialBase.h"

#include <iostream>

#include "Shader.h"
#include "models/Mesh.h"
#include "GLEWImporter.h"
#include "ShaderFactory.h"
#include <glm/gtx/hash.hpp>

std::unordered_map<size_t, std::shared_ptr<Shader>> MaterialBase::_programCache;
unsigned int MaterialBase::_idCounter = 0;
const std::string MaterialBase::MATERIAL_UPDATE_EVENT = "MATERIAL_UPDATE";


void MaterialBase::ClearProgramCache()
{
    _programCache.clear();
}

const ShaderFactory::CompiledShader& MaterialBase::BuildVertexShader() const {
    return ShaderFactory::GetCompiledShader(GL_VERTEX_SHADER, _vertexShaderPath);
}

const ShaderFactory::CompiledShader& MaterialBase::BuildFragmentShader() const {
    return ShaderFactory::GetCompiledShader(GL_FRAGMENT_SHADER, _fragmentShaderPath);
}

CullFaceMode MaterialBase::cullFace() const
{
    return _cullFace;
}

std::shared_ptr<MaterialBase> MaterialBase::Clone() const
{
    auto result = std::make_shared<MaterialBase>(*this);
    result->setId(_idCounter);
    _idCounter++;
    return result;
}

void MaterialBase::setId(unsigned int id)
{
    _id = id;
}

void MaterialBase::setShader(const std::shared_ptr<Shader>& shader)
{
    _shader = shader;
}

MaterialBase::MaterialBase(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
    : _cullFace(CullFaceMode::back), _vertexShaderPath(vertexShaderPath), _fragmentShaderPath(fragmentShaderPath)
{
    _id = _idCounter;
    _idCounter++;
}

void MaterialBase::Build()
{
    const auto& vsObject = BuildVertexShader();
    const auto& fsObject = BuildFragmentShader();

    // (опц.) Кэш программ по хешу финальных исходников
    const auto key = HashSources(vsObject.source, fsObject.source);
    if (auto it = _programCache.find(key); it != _programCache.end()) {
        _shader = it->second;   // переиспользуем
        return;
    }

    // Компилируем собственный фрагмент-шейдер
    GLuint program = LinkProgram(vsObject.id, fsObject.id);
    _shader = std::make_shared<Shader>(program);
    _programCache.emplace(key, _shader);

    std::cout << "Built shader with filters. Vertex shader:\n" << vsObject.source << "\nFragment shader:\n" << fsObject.source << std::endl;
}

GLuint MaterialBase::LinkProgram(GLuint vShader, GLuint fShader) {
    GLint success;
    GLchar infoLog[512];
    auto program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);
    // Print linking errors if any
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteProgram(program);
        throw std::runtime_error(std::string("Shader linking failed: ") + infoLog);
    }
    return program;
}

void MaterialBase::Bind(const RenderContext& ctx, const Mesh* /*mesh = nullptr*/)
{
    _shader->Use();
    switch (_cullFace)
    {
    case CullFaceMode::back:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        break;
    case CullFaceMode::front:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        break;
    default:
        glDisable(GL_CULL_FACE);
        break;

    }
}

void MaterialBase::Unbind()
{
}

size_t MaterialBase::HashSources(const std::string &vertexSource, const std::string &fragmentSource) {
    std::size_t h1 = std::hash<std::string>{}(vertexSource);
    std::size_t h2 = std::hash<std::string>{}(fragmentSource);
    glm::detail::hash_combine(h1, h2); // Combines the hash of s2 into s1's hash
    return h1;
}

std::string MaterialBase::name() const
{
    return _name;
}

void MaterialBase::setName(const std::string &name)
{
    _name = name;
}

unsigned int MaterialBase::id() const
{
    return _id;
}

void MaterialBase::setCullFace(const CullFaceMode &cullFace)
{
    _cullFace = cullFace;
}

std::shared_ptr<Shader> MaterialBase::shader() const
{
    return _shader;
}
