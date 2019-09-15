#include "GLESProgram.h"
#include <cassert>
#include <memory>
#include <sstream>
#include <stdexcept>

struct ShaderDeleter
{
    typedef GLuint pointer;
    void operator()(GLuint shader)
    {
        glDeleteShader(shader);
        assert(!glGetError());
    }
};

struct ProgramDeleter
{
    typedef GLuint pointer;
    void operator()(GLuint program)
    {
        glDeleteProgram(program);
        assert(!glGetError());
    }
};

static void PushErrString(std::ostringstream &ss, GLint error)
{
    switch (error)
    {
        case GL_NO_ERROR: ss << "GL_NO_ERROR"; break;
        case GL_INVALID_ENUM: ss << "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE: ss << "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: ss << "GL_INVALID_OPERATION"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: ss << "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        case GL_OUT_OF_MEMORY: ss << "GL_OUT_OF_MEMORY"; break;
        default: ss << "Unknown error " << error; break;
    }
}

static void ThrowGlesError(const char *msg, GLint error)
{
    if (GL_OUT_OF_MEMORY == error)
    {
        throw std::bad_alloc();
    }
    else
    {
        std::ostringstream ss;
        ss << msg << ": ";
        PushErrString(ss, error);
        throw std::runtime_error(ss.str().c_str());
    }
}

static void ThrowIfError(const char *msg)
{
    if( GLint error = glGetError() )
        ThrowGlesError(msg, error);
}

static void ThrowAlways(const char *msg)
{
    ThrowGlesError(msg, glGetError());
}

static void BindAttribLocation(GLuint program, GLuint index, const GLchar *name)
{
    glBindAttribLocation(program, index, name);
    if( GLint error = glGetError() )
    {
        std::ostringstream ss;
        ss << "Failed to bind attrib location '" << name << "': ";
        PushErrString(ss, error);
        throw std::runtime_error(ss.str().c_str());
    }
}

static std::unique_ptr<GLuint, ShaderDeleter> LoadShader(GLenum shaderType, const char *source)
{
    std::unique_ptr<GLuint, ShaderDeleter> shader(glCreateShader(shaderType));
    if (shader.get() == 0)
        ThrowAlways("Failed to create GL shader");
    
    glShaderSource(shader.get(), 1, &source, nullptr);
    ThrowIfError("Failed to set shader source");
    
    glCompileShader(shader.get());
    ThrowIfError("Failed to compile shader");
    
    GLint compileStatus = 0;
    glGetShaderiv(shader.get(), GL_COMPILE_STATUS, &compileStatus);
    ThrowIfError("Failed to get shader compile status");
    
    if (!compileStatus)
    {
        std::ostringstream ss;
        ss << "Shader compile failed:\n";
        
        GLint infoLogLength = 0;
        glGetShaderiv(shader.get(), GL_INFO_LOG_LENGTH, &infoLogLength);
        ThrowIfError("Failed to get shader info log length");
        
        if( infoLogLength > 0 )
        {
            std::string shaderInfoLog(infoLogLength, '\0');
            glGetShaderInfoLog(shader.get(), infoLogLength, nullptr, &shaderInfoLog[0]);
            ThrowIfError("Failed to get shader info log");
            ss << shaderInfoLog;
        }
        else
        {
            ss << "No shader info log available";
        }
        
        throw std::runtime_error(ss.str());
    }
    
    return shader;
}

GlesProgram::GlesProgram(const char *vertexSource,
                         const char* fragmentSource,
                         const AttribLocationBinding attribLocationBindings[])
{
    std::unique_ptr<GLuint, ShaderDeleter> vertexShader(LoadShader(GL_VERTEX_SHADER, vertexSource));
    std::unique_ptr<GLuint, ShaderDeleter> fragmentShader(LoadShader(GL_FRAGMENT_SHADER, fragmentSource));
    
    std::unique_ptr<GLuint, ProgramDeleter> program(glCreateProgram());
    if( program.get() == 0 )
        ThrowAlways("Failed to create GL program");
    
    while (attribLocationBindings->name)
    {
        BindAttribLocation(program.get(), attribLocationBindings->index, attribLocationBindings->name);
        ++attribLocationBindings;
    }
        
    glAttachShader(program.get(), vertexShader.get());
    ThrowIfError("Failed to attach vertex shader");
    
    glAttachShader(program.get(), fragmentShader.get());
    ThrowIfError("Failed to attach fragment shader");
    
    glLinkProgram(program.get());
    ThrowIfError("Failed to link program");
    
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program.get(), GL_LINK_STATUS, &linkStatus);
    ThrowIfError("Failed to get program link status");
    
    if (!linkStatus)
    {
        std::ostringstream ss;
        ss << "Program link failed:\n";
        
        GLint infoLogLength = 0;
        glGetProgramiv(program.get(), GL_INFO_LOG_LENGTH, &infoLogLength);
        ThrowIfError("Failed to get program info log length");
        
        if (infoLogLength > 0)
        {
            std::string programInfoLog(infoLogLength, '\0');
            glGetProgramInfoLog(program.get(), infoLogLength, nullptr, &programInfoLog[0]);
            ThrowIfError("Failed to get program info log");
            ss << programInfoLog;
        }
        else
        {
            ss << "No program info log available";
        }
        
        throw std::runtime_error(ss.str());
    }
    
    _program = program.release();
}

GlesProgram::~GlesProgram()
{
    glDeleteProgram(_program);
    assert(!glGetError());
}

GLint GlesProgram::GetUniformLocation(const GLchar *name) const
{
    GLint location = glGetUniformLocation(_program, name);
    ThrowIfError("Failed to get uniform location");
    return location;
}
