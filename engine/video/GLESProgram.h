#pragma once
#if __ANDROID__
#include <GLES2/gl2.h>
#else
#include <OpenGLES/ES2/gl.h>
#endif

struct AttribLocationBinding
{
    GLuint index;
    const GLchar *name;
};

class GlesProgram
{
public:
    GlesProgram(const char *vertexSource,
                const char *fragmentSource,
                const AttribLocationBinding attribLocationBindings[]);
    ~GlesProgram();

    GLint GetUniformLocation(const GLchar *name) const;
    GLuint Get() const { return _program; }

private:
    GLuint _program;
};
