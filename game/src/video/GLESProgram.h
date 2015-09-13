#pragma once
#include <OpenGLES/ES2/gl.h>

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
