#include "MaterialBase.h"

#include <iostream>

#include "Shader.h"
#include "models/Mesh.h"
#include "GLEWImporter.h"
#include "ShaderFactory.h"
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

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

    // Запоминаем финальный (сгенерированный) исходник — для запекания в префаб (8.5b).
    _compiledVertexSource = vsObject.source;
    _compiledFragmentSource = fsObject.source;

    // (опц.) Кэш программ по хешу финальных исходников
    const auto key = HashSources(vsObject.source, fsObject.source);
    if (auto it = _programCache.find(key); it != _programCache.end()) {
        _shader = it->second;   // переиспользуем
        _uniformCache.Reset(_shader->GetProgram());
        return;
    }

    // Компилируем собственный фрагмент-шейдер
    GLuint program = LinkProgram(vsObject.id, fsObject.id);
    _shader = std::make_shared<Shader>(program);
    _uniformCache.Reset(_shader->GetProgram());

    _programCache.emplace(key, _shader);

    std::cout << "Built shader with filters. Vertex shader:\n" << vsObject.source << "\nFragment shader:\n" << fsObject.source << std::endl;
}

void MaterialBase::BuildFromSources(const std::string& vertexSource, const std::string& fragmentSource)
{
    // Прямая компиляция готового кода БЕЗ инъекции фильтров/лайтинга (compiled-путь).
    const auto& vsObject = ShaderFactory::GetCompiledShaderFromSource(GL_VERTEX_SHADER, vertexSource);
    const auto& fsObject = ShaderFactory::GetCompiledShaderFromSource(GL_FRAGMENT_SHADER, fragmentSource);

    _compiledVertexSource = vertexSource;
    _compiledFragmentSource = fragmentSource;

    const auto key = HashSources(vertexSource, fragmentSource);
    if (auto it = _programCache.find(key); it != _programCache.end()) {
        _shader = it->second;
    } else {
        GLuint program = LinkProgram(vsObject.id, fsObject.id);
        _shader = std::make_shared<Shader>(program);
        _programCache.emplace(key, _shader);
    }
    _uniformCache.Reset(_shader->GetProgram());
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

    GLint modelLoc = _uniformCache.GetUniformLocation("model");
    GLint viewLoc = _uniformCache.GetUniformLocation("view");
    GLint projectionLoc = _uniformCache.GetUniformLocation("projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ctx.model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(ctx.view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(ctx.projection));
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