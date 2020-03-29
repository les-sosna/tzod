#pragma once
#include "RenderBase.h"
#include "detail/GLESProgram.h"
#if __ANDROID__
#include <GLES2/gl2.h>
#else
#include <OpenGLES/ES2/gl.h>
#endif

#define VERTEX_ARRAY_SIZE   1024
#define  INDEX_ARRAY_SIZE   2048


class RenderGLES2 : public IRender
{
public:
    RenderGLES2();
    ~RenderGLES2() override;

    void Begin(unsigned int displayWidth, unsigned int displayHeight, DisplayOrientation displayOrientation);
    void End();

    // IRender
    void SetViewport(const RectRB &rect) override;
    void SetScissor(const RectRB &rect) override;
    void SetTransform(vec2d offset, float scale) override;

    void SetMode(const RenderMode mode) override;

    void SetAmbient(float ambient) override;

    bool TexCreate(DEV_TEXTURE &tex, ImageView img, bool magFilter) override;
    void TexFree(DEV_TEXTURE tex) override;

    MyVertex* DrawQuad(DEV_TEXTURE tex) override;
    MyVertex* DrawFan(unsigned int nEdges) override;

private:
    void Flush();

    unsigned int _windowHeight = 0;
    RectRB _rtViewport;
    vec2d _offset = {};
    float _scale = 1;

    GLuint _curtex = -1;
    float _ambient;

    GLushort _IndexArray[INDEX_ARRAY_SIZE];
    MyVertex _VertexArray[VERTEX_ARRAY_SIZE];

    unsigned int _vaSize = 0;      // number of filled elements in _VertexArray
    unsigned int _iaSize = 0;      // number of filled elements in _IndexArray

    RenderMode _mode;

    GlesProgram _program;
    GLint _viewProj;
    GLint _sampler;
};
