#ifndef GL_UTILS_H
#define GL_UTILS_H
#include <GL/glew.h>
//#include "glad/glad.h"

class GLUtils
{
public:
	GLUtils();
	~GLUtils();

	static GLenum glCheckError_(const char *file, int line);
};

#define glCheckError() GLUtils::glCheckError_(__FILE__, __LINE__) 

#endif

		

