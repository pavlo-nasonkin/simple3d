#pragma once
#include "GL/glew.h"

struct VertexAttribute
{
    GLuint   location;          // layout(location=N) в шейдере
    GLint    componentCount;    // 1, 2, 3, или 4
    GLenum   componentType;     // GL_FLOAT, GL_INT, GL_UNSIGNED_BYTE, ...
    GLboolean normalize;        // нормализовать в [0,1] / [-1,1]?
    size_t   offset;            // байт от начала вершины
    bool     isInteger;         // true → glVertexAttribIPointer (для boneIds)
};
