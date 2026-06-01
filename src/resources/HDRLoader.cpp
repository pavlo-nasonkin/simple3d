#include "HDRLoader.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <iostream>

#include "TextureCube.h"
#include "render/GLBuffer.h"
#include "render/GLVertexArray.h"

namespace {

// ───────────────────────── Radiance .hdr (RGBE) loader ─────────────────────────
// Самодостаточный парсер: header + new-format RLE (и flat-fallback). Возвращает
// массив float RGB (3 канала на пиксель) или nullptr. Строки кладутся снизу-вверх
// (GL-строка 0 = низ), чтобы equirect не оказался перевёрнут по вертикали.

bool readHDRHeader(FILE* f, int& width, int& height)
{
    char line[512];
    if (!std::fgets(line, sizeof(line), f)) return false;
    if (line[0] != '#' || line[1] != '?') return false; // не RADIANCE/RGBE

    bool formatOk = false;
    while (std::fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') break; // конец header'а
        if (std::strncmp(line, "FORMAT=", 7) == 0) {
            if (std::strstr(line, "32-bit_rle_rgbe") || std::strstr(line, "32-bit_rle_xyze")) {
                formatOk = true;
            }
        }
    }
    if (!formatOk) return false;

    if (!std::fgets(line, sizeof(line), f)) return false;
    int h = 0, w = 0;
    if (std::sscanf(line, "-Y %d +X %d", &h, &w) != 2) return false; // поддерживаем только этот порядок
    if (w <= 0 || h <= 0) return false;
    width = w;
    height = h;
    return true;
}

bool readFlatScanline(FILE* f, const unsigned char first[4], unsigned char* scan, int width)
{
    scan[0] = first[0]; scan[1] = first[1]; scan[2] = first[2]; scan[3] = first[3];
    for (int x = 1; x < width; ++x) {
        if (std::fread(&scan[x * 4], 1, 4, f) != 4) return false;
    }
    return true;
}

bool readRLEScanline(FILE* f, unsigned char* scan /*width*4, interleaved*/, int width)
{
    for (int c = 0; c < 4; ++c) {
        int x = 0;
        while (x < width) {
            unsigned char code[2];
            if (std::fread(code, 1, 2, f) != 2) return false;

            if (code[0] > 128) {
                // run: повтор одного байта (code[0]-128) раз
                const int count = code[0] - 128;
                if (count == 0 || x + count > width) return false;
                for (int i = 0; i < count; ++i) scan[(x++) * 4 + c] = code[1];
            } else {
                // dump: code[0] литералов; первый уже прочитан в code[1]
                int count = code[0];
                if (count == 0 || x + count > width) return false;
                scan[(x++) * 4 + c] = code[1];
                for (int i = 1; i < count; ++i) {
                    unsigned char v;
                    if (std::fread(&v, 1, 1, f) != 1) return false;
                    scan[(x++) * 4 + c] = v;
                }
            }
        }
    }
    return true;
}

float* loadHDR(const std::string& path, int& outW, int& outH)
{
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return nullptr;

    int width = 0, height = 0;
    if (!readHDRHeader(f, width, height)) {
        std::fclose(f);
        return nullptr;
    }

    float* data = new float[static_cast<size_t>(width) * height * 3];
    std::vector<unsigned char> scan(static_cast<size_t>(width) * 4);
    bool ok = true;

    for (int y = 0; y < height && ok; ++y) {
        unsigned char head[4];
        if (std::fread(head, 1, 4, f) != 4) { ok = false; break; }

        const bool isRLE = !(width < 8 || width > 0x7fff) &&
                           head[0] == 2 && head[1] == 2 && (head[2] & 0x80) == 0;
        if (isRLE) {
            const int len = (head[2] << 8) | head[3];
            if (len != width) { ok = false; break; }
            ok = readRLEScanline(f, scan.data(), width);
        } else {
            ok = readFlatScanline(f, head, scan.data(), width);
        }
        if (!ok) break;

        // строки .hdr идут сверху-вниз; пишем перевёрнуто, чтобы GL-строка 0 была низом
        const int dstRow = height - 1 - y;
        float* dst = data + static_cast<size_t>(dstRow) * width * 3;
        for (int x = 0; x < width; ++x) {
            const unsigned char r = scan[x * 4 + 0];
            const unsigned char g = scan[x * 4 + 1];
            const unsigned char b = scan[x * 4 + 2];
            const unsigned char e = scan[x * 4 + 3];
            if (e != 0) {
                const float fexp = std::ldexp(1.0f, static_cast<int>(e) - (128 + 8));
                dst[x * 3 + 0] = r * fexp;
                dst[x * 3 + 1] = g * fexp;
                dst[x * 3 + 2] = b * fexp;
            } else {
                dst[x * 3 + 0] = dst[x * 3 + 1] = dst[x * 3 + 2] = 0.0f;
            }
        }
    }

    std::fclose(f);
    if (!ok) { delete[] data; return nullptr; }

    outW = width;
    outH = height;
    return data;
}

// ───────────────────────── shaders + cube для бейка ─────────────────────────

const char* kVertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 projection;
uniform mat4 view;
out vec3 localPos;
void main() {
    localPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

const char* kFragmentSrc = R"(#version 330 core
out vec4 FragColor;
in vec3 localPos;
uniform sampler2D equirectangularMap;
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}
void main() {
    vec2 uv = SampleSphericalMap(normalize(localPos));
    FragColor = vec4(texture(equirectangularMap, uv).rgb, 1.0);
}
)";

const float kCubeVertices[] = {
    // positions (36 verts, two tris per face)
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

GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cout << "[HDRLoader] shader compile error: " << log << std::endl;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint compileProgram(const char* vs, const char* fs)
{
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
        char log[512];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        std::cout << "[HDRLoader] program link error: " << log << std::endl;
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

} // namespace

std::shared_ptr<TextureCube> HDRLoader::EquirectFileToCubemap(const std::string& path, int faceSize)
{
    int w = 0, h = 0;
    float* pixels = loadHDR(path, w, h);
    if (!pixels) {
        std::cout << "[HDRLoader] failed to load HDR: " << path << std::endl;
        return nullptr;
    }

    // 1. equirect → временная float-текстура GL_RGB16F
    GLuint equirectTex = 0;
    glGenTextures(1, &equirectTex);
    glBindTexture(GL_TEXTURE_2D, equirectTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    delete[] pixels;

    // 2. целевой пустой cubemap
    TextureCube cubemap = TextureCube::CreateEmpty(faceSize, GL_RGB16F, GL_RGB, GL_FLOAT, false);

    GLuint program = compileProgram(kVertexSrc, kFragmentSrc);
    if (!program) {
        glDeleteTextures(1, &equirectTex);
        return nullptr;
    }

    // 3. геометрия куба
    GLVertexArray vao;
    GLBuffer vbo;
    vao.Bind();
    vbo.SetData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    GLVertexArray::Unbind();

    // 4. FBO + depth RBO
    GLuint fbo = 0, rbo = 0;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, faceSize, faceSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const glm::mat4 captureViews[6] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    };

    // сохранить GL-состояние, которое меняем
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    GLint prevFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);

    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, equirectTex);
    glUniform1i(glGetUniformLocation(program, "equirectangularMap"), 0);
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
    const GLint viewLoc = glGetUniformLocation(program, "view");

    glDisable(GL_CULL_FACE); // куб смотрим изнутри — иначе грани отсекутся
    glViewport(0, 0, faceSize, faceSize);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    vao.Bind();
    for (int i = 0; i < 6; ++i) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap.Id(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    GLVertexArray::Unbind();

    // восстановить состояние
    glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    if (cullWasEnabled) glEnable(GL_CULL_FACE);

    // очистить временные ресурсы (vao/vbo удалятся RAII при выходе)
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &equirectTex);
    glDeleteProgram(program);
    glBindTexture(GL_TEXTURE_2D, 0);

    return std::make_shared<TextureCube>(std::move(cubemap));
}
