#pragma once
#include "GL/glew.h"

// Requires an active GL context during construction and destruction.
// Do not create as static/global objects.
class GLVertexArray {
    GLuint _id = 0;
public:
    GLVertexArray()  { glGenVertexArrays(1, &_id); }
    ~GLVertexArray() { if (_id) glDeleteVertexArrays(1, &_id); }

    GLVertexArray(const GLVertexArray&) = delete;
    GLVertexArray& operator=(const GLVertexArray&) = delete;
    GLVertexArray(GLVertexArray&& other) noexcept : _id(other._id) { other._id = 0; }

    GLVertexArray& operator=(GLVertexArray&& other) noexcept {
        if (this != &other)
        {
            if (_id)
            {
                glDeleteVertexArrays(1, &_id);
            }
            _id = other._id;
            other._id = 0;
        }
        return *this;
    }

    GLuint Id() const { return _id; }
    void Bind() const {
        glBindVertexArray(_id);
    }

    static void Unbind() {
        glBindVertexArray(0);
    }
};
