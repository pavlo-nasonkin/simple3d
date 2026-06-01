#include "IBLBaker.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <cmath>

#include "resources/TextureCube.h"
#include "resources/Texture2D.h"
#include "render/GLBuffer.h"
#include "render/GLVertexArray.h"

namespace {

// ───────────────────────── общая геометрия / матрицы ─────────────────────────

const float kCubeVertices[] = {
    -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,
};

// pos.xy + uv для fullscreen-quad (triangle strip)
const float kQuadVertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
};

glm::mat4 captureProjection() {
    return glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
}

void captureViews(glm::mat4 out[6]) {
    out[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
    out[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
    out[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f));
    out[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f));
    out[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
    out[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
}

// ───────────────────────── компиляция шейдеров ─────────────────────────

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cout << "[IBLBaker] shader compile error: " << log << std::endl;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint compileProgram(const char* vs, const char* fs) {
    GLuint v = compileShader(GL_VERTEX_SHADER, vs);
    if (!v) return 0;
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs);
    if (!f) { glDeleteShader(v); return 0; }
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        std::cout << "[IBLBaker] program link error: " << log << std::endl;
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

// ───────────────────────── GLSL ─────────────────────────

const char* kCubeVS = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 projection;
uniform mat4 view;
out vec3 localPos;
void main() {
    localPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

const char* kIrradianceFS = R"(#version 330 core
out vec4 FragColor;
in vec3 localPos;
uniform samplerCube environmentMap;
const float PI = 3.14159265359;
void main() {
    vec3 N = normalize(localPos);
    vec3 irradiance = vec3(0.0);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / nrSamples);
    FragColor = vec4(irradiance, 1.0);
}
)";

const char* kImportanceCommon = R"(
const float PI = 3.14159265359;
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}
vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}
)";

std::string makePrefilterFS() {
    return std::string("#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 localPos;\n"
        "uniform samplerCube environmentMap;\n"
        "uniform float roughness;\n")
        + kImportanceCommon +
        "void main() {\n"
        "    vec3 N = normalize(localPos);\n"
        "    vec3 R = N;\n"
        "    vec3 V = R;\n"
        "    const uint SAMPLE_COUNT = 1024u;\n"
        "    vec3 prefilteredColor = vec3(0.0);\n"
        "    float totalWeight = 0.0;\n"
        "    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {\n"
        "        vec2 Xi = Hammersley(i, SAMPLE_COUNT);\n"
        "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
        "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
        "        float NdotL = max(dot(N, L), 0.0);\n"
        "        if (NdotL > 0.0) {\n"
        "            prefilteredColor += texture(environmentMap, L).rgb * NdotL;\n"
        "            totalWeight += NdotL;\n"
        "        }\n"
        "    }\n"
        "    prefilteredColor = prefilteredColor / max(totalWeight, 0.001);\n"
        "    FragColor = vec4(prefilteredColor, 1.0);\n"
        "}\n";
}

const char* kBrdfVS = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
out vec2 UV;
void main() {
    UV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

std::string makeBrdfFS() {
    return std::string("#version 330 core\n"
        "out vec2 FragColor;\n"
        "in vec2 UV;\n")
        + kImportanceCommon +
        "float GeometrySchlickGGX(float NdotV, float roughness) {\n"
        "    float a = roughness;\n"
        "    float k = (a * a) / 2.0;\n"
        "    return NdotV / (NdotV * (1.0 - k) + k);\n"
        "}\n"
        "float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {\n"
        "    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) *\n"
        "           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);\n"
        "}\n"
        "vec2 IntegrateBRDF(float NdotV, float roughness) {\n"
        "    vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);\n"
        "    float A = 0.0;\n"
        "    float B = 0.0;\n"
        "    vec3 N = vec3(0.0, 0.0, 1.0);\n"
        "    const uint SAMPLE_COUNT = 1024u;\n"
        "    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {\n"
        "        vec2 Xi = Hammersley(i, SAMPLE_COUNT);\n"
        "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
        "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
        "        float NdotL = max(L.z, 0.0);\n"
        "        float NdotH = max(H.z, 0.0);\n"
        "        float VdotH = max(dot(V, H), 0.0);\n"
        "        if (NdotL > 0.0) {\n"
        "            float G = GeometrySmith(N, V, L, roughness);\n"
        "            float G_Vis = (G * VdotH) / (NdotH * NdotV);\n"
        "            float Fc = pow(1.0 - VdotH, 5.0);\n"
        "            A += (1.0 - Fc) * G_Vis;\n"
        "            B += Fc * G_Vis;\n"
        "        }\n"
        "    }\n"
        "    return vec2(A, B) / float(SAMPLE_COUNT);\n"
        "}\n"
        "void main() {\n"
        "    FragColor = IntegrateBRDF(UV.x, UV.y);\n"
        "}\n";
}

// сохранение/восстановление затрагиваемого GL-состояния
struct GLStateGuard {
    GLint viewport[4];
    GLint fbo = 0;
    GLboolean cull = GL_FALSE;
    GLStateGuard() {
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
        cull = glIsEnabled(GL_CULL_FACE);
    }
    ~GLStateGuard() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        if (cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    }
};

} // namespace

std::shared_ptr<TextureCube> IBLBaker::BakeIrradiance(const TextureCube& env, int faceSize)
{
    GLuint program = compileProgram(kCubeVS, kIrradianceFS);
    if (!program) return nullptr;

    TextureCube irradiance = TextureCube::CreateEmpty(faceSize, GL_RGB16F, GL_RGB, GL_FLOAT, false);

    GLVertexArray vao;
    GLBuffer vbo;
    vao.Bind();
    vbo.SetData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    GLVertexArray::Unbind();

    GLuint fbo = 0, rbo = 0;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, faceSize, faceSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    glm::mat4 views[6];
    captureViews(views);

    {
        GLStateGuard guard;
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glDisable(GL_CULL_FACE);

        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection()));
        glUniform1i(glGetUniformLocation(program, "environmentMap"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, env.Id());
        const GLint viewLoc = glGetUniformLocation(program, "view");

        glViewport(0, 0, faceSize, faceSize);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        vao.Bind();
        for (int i = 0; i < 6; ++i) {
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(views[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradiance.Id(), 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        GLVertexArray::Unbind();
    }

    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
    glDeleteProgram(program);

    return std::make_shared<TextureCube>(std::move(irradiance));
}

std::shared_ptr<TextureCube> IBLBaker::BakePrefiltered(const TextureCube& env, int baseFaceSize, int mipLevels)
{
    const std::string fsSrc = makePrefilterFS();
    GLuint program = compileProgram(kCubeVS, fsSrc.c_str());
    if (!program) return nullptr;

    // env нужно сэмплировать с мипами — иначе на больших roughness видны точки
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.Id());
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    TextureCube prefiltered = TextureCube::CreateEmpty(baseFaceSize, GL_RGB16F, GL_RGB, GL_FLOAT, true);

    GLVertexArray vao;
    GLBuffer vbo;
    vao.Bind();
    vbo.SetData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    GLVertexArray::Unbind();

    GLuint fbo = 0, rbo = 0;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);

    glm::mat4 views[6];
    captureViews(views);

    {
        GLStateGuard guard;
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glDisable(GL_CULL_FACE);

        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection()));
        glUniform1i(glGetUniformLocation(program, "environmentMap"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, env.Id());
        const GLint viewLoc = glGetUniformLocation(program, "view");
        const GLint roughLoc = glGetUniformLocation(program, "roughness");

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        vao.Bind();
        for (int mip = 0; mip < mipLevels; ++mip) {
            const int mipSize = static_cast<int>(baseFaceSize * std::pow(0.5, mip));
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
            glViewport(0, 0, mipSize, mipSize);

            const float roughness = (mipLevels > 1) ? static_cast<float>(mip) / (mipLevels - 1) : 0.0f;
            glUniform1f(roughLoc, roughness);

            for (int i = 0; i < 6; ++i) {
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(views[i]));
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefiltered.Id(), mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
        GLVertexArray::Unbind();
    }

    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
    glDeleteProgram(program);

    return std::make_shared<TextureCube>(std::move(prefiltered));
}

std::shared_ptr<Texture2D> IBLBaker::BakeBRDFLUT(int size)
{
    const std::string fsSrc = makeBrdfFS();
    GLuint program = compileProgram(kBrdfVS, fsSrc.c_str());
    if (!program) return nullptr;

    GLuint lutId = 0;
    glGenTextures(1, &lutId);
    glBindTexture(GL_TEXTURE_2D, lutId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size, size, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLVertexArray vao;
    GLBuffer vbo;
    vao.Bind();
    vbo.SetData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    GLVertexArray::Unbind();

    GLuint fbo = 0, rbo = 0;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lutId, 0);

    {
        GLStateGuard guard;
        glDisable(GL_CULL_FACE);
        glViewport(0, 0, size, size);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glUseProgram(program);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        vao.Bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        GLVertexArray::Unbind();
    }

    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
    glDeleteProgram(program);
    glBindTexture(GL_TEXTURE_2D, 0);

    auto lut = std::make_shared<Texture2D>();
    lut->id = lutId;
    lut->type = "brdf_lut";
    return lut;
}
