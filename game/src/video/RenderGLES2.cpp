#include "GLESProgram.h"
#include "inc/video/RenderOpenGL.h"
#include <OpenGLES/ES2/gl.h>
#include <cstring>

#define VERTEX_ARRAY_SIZE   1024
#define  INDEX_ARRAY_SIZE   2048


class RenderGLES2 : public IRender
{
    int _windowWidth;
    int _windowHeight;
	RectRB   _rtViewport;

	GLuint _curtex;
	float  _ambient;

	GLushort _IndexArray[INDEX_ARRAY_SIZE];
	MyVertex _VertexArray[VERTEX_ARRAY_SIZE];

	size_t    _vaSize;      // number of filled elements in _VertexArray
	size_t    _iaSize;      // number of filled elements in _IndexArray

	RenderMode  _mode;

    GlesProgram _program;
    GLuint _proj;
    GLuint _sampler;

public:
	RenderGLES2();
	virtual ~RenderGLES2() override;

private:
	void Flush();

	virtual void OnResizeWnd(unsigned int width, unsigned int height);

	virtual void SetViewport(const RectRB *rect);
	virtual void SetScissor(const RectRB *rect);
	virtual void Camera(const RectRB *vp, float x, float y, float scale);

	virtual int  GetWidth() const;
	virtual int  GetHeight() const;

	virtual int  GetViewportWidth() const;
	virtual int  GetViewportHeight() const;

	virtual void Begin(void);
	virtual void End(void);
	virtual void SetMode (const RenderMode mode);

	virtual void SetAmbient(float ambient);


	//
	// texture management
	//

	virtual bool TexCreate(DEV_TEXTURE &tex, const Image &img);
	virtual void TexFree(DEV_TEXTURE tex);

	virtual MyVertex* DrawQuad(DEV_TEXTURE tex);
	virtual MyVertex* DrawFan(size_t nEdges);

	virtual void DrawLines(const MyLine *lines, size_t count);
};

///////////////////////////////////////////////////////////////////////////////


static const char s_vertexShader[] =
R"(
    attribute vec2 vPos;
    attribute vec2 vTexCoord;
    attribute vec4 vColor;
    varying lowp vec2 texCoord;
    varying lowp vec4 color;
    uniform vec2 vProj;
    void main()
    {
        gl_Position = vec4(vPos.x * vProj.x * 2.0 - 1.0, 1.0 - vPos.y * vProj.y * 2.0, 0, 1);
        texCoord = vTexCoord;
        color = vColor;
    }
)";

static const char s_fragmentShader[] =
R"(
    varying lowp vec2 texCoord;
    varying lowp vec4 color;
    uniform sampler2D s_tex;
    void main()
    {
        gl_FragColor = texture2D(s_tex, texCoord) * color;
    }
)";


#define ATTRIB_POSITION     0
#define ATTRIB_TEXCOORD     1
#define ATTRIB_COLOR        2

static const AttribLocationBinding s_bindings[] =
{
    {ATTRIB_POSITION, "vPos"},
    {ATTRIB_COLOR, "vColor"},
    {ATTRIB_TEXCOORD, "vTexCoord"},
    {0, nullptr},
};

///////////////////////////////////////////////////////////////////////////////

RenderGLES2::RenderGLES2()
    : _windowWidth(0)
    , _windowHeight(0)
    , _curtex(-1)
    , _vaSize(0)
    , _iaSize(0)
    , _program(s_vertexShader, s_fragmentShader, s_bindings)
    , _proj(_program.GetUniformLocation("vProj"))
    , _sampler(_program.GetUniformLocation("s_tex"))
{
	memset(_IndexArray, 0, sizeof(GLushort) * INDEX_ARRAY_SIZE);
	memset(_VertexArray, 0, sizeof(MyVertex) * VERTEX_ARRAY_SIZE);
}

RenderGLES2::~RenderGLES2()
{
}

void RenderGLES2::OnResizeWnd(unsigned int width, unsigned int height)
{
	_windowWidth = (int) width;
    _windowHeight = (int) height;
    SetViewport(nullptr);
    SetScissor(nullptr);
}

void RenderGLES2::SetScissor(const RectRB *rect)
{
	Flush();
	if( rect )
	{
		glScissor(rect->left, _windowHeight - rect->bottom, rect->right - rect->left, rect->bottom - rect->top);
		glEnable(GL_SCISSOR_TEST);
	}
	else
	{
		glDisable(GL_SCISSOR_TEST);
	}
}

void RenderGLES2::SetViewport(const RectRB *rect)
{
	Flush();

//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
	if( rect )
	{
//		glOrtho(0, (GLdouble) (rect->right - rect->left), (GLdouble) (rect->bottom - rect->top), 0, -1, 1);
		glViewport(
			rect->left,                       // X
			_windowHeight - rect->bottom,     // Y
			rect->right - rect->left,         // width
			rect->bottom - rect->top          // height
			);
		_rtViewport = *rect;
	}
	else
	{
//		glOrtho(0, (GLdouble) _windowWidth, (GLdouble) _windowHeight, 0, -1, 1);
		glViewport(0, 0, _windowWidth, _windowHeight);
		_rtViewport.left   = 0;
		_rtViewport.top    = 0;
		_rtViewport.right  = _windowWidth;
		_rtViewport.bottom = _windowHeight;
	}
}

void RenderGLES2::Camera(const RectRB *vp, float x, float y, float scale)
{
	SetViewport(vp);
	SetScissor(vp);
/*
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(scale, scale, 1);
	if (vp)
		glTranslatef((float) WIDTH(*vp) / 2 / scale - x, (float) HEIGHT(*vp) / 2 / scale - y, 0);
	else
		glTranslatef(0, 0, 0);*/
}

int RenderGLES2::GetWidth() const
{
	return _windowWidth;
}

int RenderGLES2::GetHeight() const
{
	return _windowHeight;
}

int RenderGLES2::GetViewportWidth() const
{
	return _rtViewport.right - _rtViewport.left;
}

int RenderGLES2::GetViewportHeight() const
{
	return _rtViewport.bottom - _rtViewport.top;
}

void RenderGLES2::Begin()
{
    glEnable(GL_BLEND);
    
    glVertexAttribPointer(ATTRIB_POSITION, 2, GL_FLOAT, false, sizeof(MyVertex), &_VertexArray->x);
    glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, false, sizeof(MyVertex), &_VertexArray->u);
    glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, true, sizeof(MyVertex), &_VertexArray->color);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glEnableVertexAttribArray(ATTRIB_TEXCOORD);
    glEnableVertexAttribArray(ATTRIB_COLOR);

	glClearColor(0, 0, 0, _ambient);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT);
}

void RenderGLES2::End()
{
	Flush();
}

void RenderGLES2::SetMode(const RenderMode mode)
{
	Flush();

	switch( mode )
	{
	case RM_LIGHT:
		glClearColor(0, 0, 0, _ambient);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		break;

	case RM_WORLD:
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		break;

	case RM_INTERFACE:
		SetViewport(NULL);
		Camera(NULL, 0, 0, 1);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		break;

	default:
		assert(false);
	}

	_mode = mode;
}

bool RenderGLES2::TexCreate(DEV_TEXTURE &tex, const Image &img)
{
	glGenTextures(1, &tex.index);
	glBindTexture(GL_TEXTURE_2D, tex.index);

	glTexImage2D(
		GL_TEXTURE_2D,                      // target
		0,                                  // level
		(24==img.GetBpp())?GL_RGB:GL_RGBA,  // internalformat
		img.GetWidth(),                     // width
		img.GetHeight(),                    // height
		0,                                  // border
		(24==img.GetBpp())?GL_RGB:GL_RGBA,  // format
		GL_UNSIGNED_BYTE,                   // type
		img.GetData()                       // pixels
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_NEAREST*/ GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_NEAREST*/ GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return true;
}

void RenderGLES2::TexFree(DEV_TEXTURE tex)
{
	assert(glIsTexture(tex.index));
	glDeleteTextures(1, &tex.index);
}

void RenderGLES2::Flush()
{
	if( _iaSize )
	{
		glDrawElements(GL_TRIANGLES, _iaSize, GL_UNSIGNED_SHORT, _IndexArray);
		_vaSize = _iaSize = 0;
	}
}

MyVertex* RenderGLES2::DrawQuad(DEV_TEXTURE tex)
{
	if( _curtex != reinterpret_cast<GLuint&>(tex.index) )
	{
		Flush();
		_curtex = reinterpret_cast<GLuint&>(tex.index);
		glBindTexture(GL_TEXTURE_2D, _curtex);
	}
	if( _vaSize > VERTEX_ARRAY_SIZE - 4 || _iaSize > INDEX_ARRAY_SIZE  - 6 )
	{
		Flush();
	}

	MyVertex *result = &_VertexArray[_vaSize];

    _IndexArray[_iaSize]   = _vaSize;
    _IndexArray[_iaSize+1] = _vaSize+1;
    _IndexArray[_iaSize+2] = _vaSize+2;
    _IndexArray[_iaSize+3] = _vaSize;
    _IndexArray[_iaSize+4] = _vaSize+2;
    _IndexArray[_iaSize+5] = _vaSize+3;

	_iaSize += 6;
	_vaSize += 4;

	return result;
}

MyVertex* RenderGLES2::DrawFan(size_t nEdges)
{
	assert(nEdges*3 < INDEX_ARRAY_SIZE);

	if( _vaSize + nEdges   > VERTEX_ARRAY_SIZE - 1 ||
		_iaSize + nEdges*3 > INDEX_ARRAY_SIZE )
	{
		Flush();
	}

	MyVertex *result = &_VertexArray[_vaSize];

	for( size_t i = 0; i < nEdges; ++i )
	{
		_IndexArray[_iaSize + i*3    ] = _vaSize;
		_IndexArray[_iaSize + i*3 + 1] = _vaSize + i + 1;
		_IndexArray[_iaSize + i*3 + 2] = _vaSize + i + 2;
	}
	_IndexArray[_iaSize + nEdges*3 - 1] = _vaSize + 1;

	_iaSize += nEdges*3;
	_vaSize += nEdges+1;

	return result;
}

void RenderGLES2::DrawLines(const MyLine *lines, size_t count)
{
	Flush();

	glDisable(GL_TEXTURE_2D);
/*
	glBegin(GL_LINES);
	const MyLine *it = lines, *end = lines + count;
	for( ; it != end; ++it )
	{
		glColor4ub(it->color.rgba[3], it->color.rgba[2], it->color.rgba[1], it->color.rgba[0]);
		glVertex2fv((const float *) &it->begin);
		glVertex2fv((const float *) &it->end);
	}
	glEnd();
*/
	SetMode(_mode); // to enable texture
}

void RenderGLES2::SetAmbient(float ambient)
{
	_ambient = ambient;
}


//-----------------------------------------------------------------------------

std::unique_ptr<IRender> RenderCreateOpenGL()
{
	return std::unique_ptr<IRender>(new RenderGLES2());
}

