#pragma once
#include "GL/glew.h"

// Requires an active GL context during construction and destruction.
// Do not create as static/global objects.
class GLBuffer {
    GLuint _id = 0;
public:
    GLBuffer()  { glGenBuffers(1, &_id); }
    ~GLBuffer() { if (_id) glDeleteBuffers(1, &_id); }

    GLBuffer(const GLBuffer&) = delete;
    GLBuffer& operator=(const GLBuffer&) = delete;
    GLBuffer(GLBuffer&& other) noexcept : _id(other._id) { other._id = 0; }

    GLBuffer& operator=(GLBuffer&& other) noexcept {
        if (this != &other)
        {
            if (_id)
            {
                glDeleteBuffers(1, &_id);
            }
            _id = other._id;
            other._id = 0;
        }
        return *this;
    }

    GLuint Id() const { return _id; }

    void Bind(GLenum target) const {
        glBindBuffer(target, _id);
    }

    void SetData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) const {
        glBindBuffer(target, _id);
        glBufferData(target, size, data, usage);
    }

};
