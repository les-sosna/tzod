#include "inc/video/RenderOpenGL.h"
#include <GLFW/glfw3.h>
#include <cstring>

#define VERTEX_ARRAY_SIZE   1024
#define  INDEX_ARRAY_SIZE   2048


class RenderOpenGL : public IRender
{
    int _windowWidth;
    int _windowHeight;
	RectRB   _rtViewport;

	GLuint _curtex;
	float  _ambient;

	GLushort _IndexArray[INDEX_ARRAY_SIZE];
	MyVertex _VertexArray[VERTEX_ARRAY_SIZE];

	unsigned int _vaSize;      // number of filled elements in _VertexArray
	unsigned int _iaSize;      // number of filled elements in _IndexArray

	RenderMode  _mode;

public:
	RenderOpenGL();
	~RenderOpenGL() override;

private:
	void Flush();

	void OnResizeWnd(unsigned int width, unsigned int height) override;
	void SetDisplayOrientation(DisplayOrientation displayOrientation) override { assert(DO_0 == displayOrientation); }

	void SetViewport(const RectRB *rect) override;
	void SetScissor(const RectRB *rect) override;
	void Camera(const RectRB *vp, float x, float y, float scale) override;

	void Begin(void) override;
	void End(void) override;
	void SetMode (const RenderMode mode) override;

	void SetAmbient(float ambient) override;


	//
	// texture management
	//

	bool TexCreate(DEV_TEXTURE &tex, const Image &img) override;
	void TexFree(DEV_TEXTURE tex) override;

	MyVertex* DrawQuad(DEV_TEXTURE tex) override;
	MyVertex* DrawFan(unsigned int nEdges) override;

	void DrawLines(const MyLine *lines, size_t count) override;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

RenderOpenGL::RenderOpenGL()
    : _windowWidth(0)
    , _windowHeight(0)
    , _curtex(-1)
    , _vaSize(0)
    , _iaSize(0)
{
	memset(_IndexArray, 0, sizeof(GLushort) * INDEX_ARRAY_SIZE);
	memset(_VertexArray, 0, sizeof(MyVertex) * VERTEX_ARRAY_SIZE);
}

RenderOpenGL::~RenderOpenGL()
{
}

void RenderOpenGL::OnResizeWnd(unsigned int width, unsigned int height)
{
	_windowWidth = (int) width;
    _windowHeight = (int) height;
    SetViewport(nullptr);
    SetScissor(nullptr);
}

void RenderOpenGL::SetScissor(const RectRB *rect)
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

void RenderOpenGL::SetViewport(const RectRB *rect)
{
	Flush();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if( rect )
	{
		glOrtho(0, (GLdouble) (rect->right - rect->left),
			(GLdouble) (rect->bottom - rect->top), 0, -1, 1);
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
		glOrtho(0, (GLdouble) _windowWidth, (GLdouble) _windowHeight, 0, -1, 1);
		glViewport(0, 0, _windowWidth, _windowHeight);
		_rtViewport.left   = 0;
		_rtViewport.top    = 0;
		_rtViewport.right  = _windowWidth;
		_rtViewport.bottom = _windowHeight;
	}
}

void RenderOpenGL::Camera(const RectRB *vp, float x, float y, float scale)
{
	SetViewport(vp);
	SetScissor(vp);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (vp)
		glTranslatef((float) (WIDTH(*vp) / 2) - x * scale, (float) (HEIGHT(*vp) / 2) - y * scale, 0);
	else
		glTranslatef(0, 0, 0);
	glScalef(scale, scale, 1);
}

void RenderOpenGL::Begin()
{
    glEnable(GL_BLEND);

    glTexCoordPointer(2, GL_FLOAT,         sizeof(MyVertex), &_VertexArray->u    );
    glColorPointer   (4, GL_UNSIGNED_BYTE, sizeof(MyVertex), &_VertexArray->color);
    glVertexPointer  (2, GL_FLOAT,         sizeof(MyVertex), &_VertexArray->x    );

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

	glClearColor(0, 0, 0, _ambient);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT);
}

void RenderOpenGL::End()
{
	Flush();
}

void RenderOpenGL::SetMode(const RenderMode mode)
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
		SetViewport(nullptr);
		Camera(nullptr, 0, 0, 1);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		break;

	default:
		assert(false);
	}

	_mode = mode;
}

bool RenderOpenGL::TexCreate(DEV_TEXTURE &tex, const Image &img)
{
	glGenTextures(1, &tex.index);
	glBindTexture(GL_TEXTURE_2D, tex.index);

	glTexImage2D(
		GL_TEXTURE_2D,                      // target
		0,                                  // level
		GL_RGBA,                            // internalformat
		img.GetWidth(),                    // width
		img.GetHeight(),                   // height
		0,                                  // border
		(24==img.GetBpp())?GL_RGB:GL_RGBA, // format
		GL_UNSIGNED_BYTE,                   // type
		img.GetData()                      // pixels
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_NEAREST*/ GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_NEAREST*/ GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return true;
}

void RenderOpenGL::TexFree(DEV_TEXTURE tex)
{
	assert(glIsTexture(tex.index));
	glDeleteTextures(1, &tex.index);
}

void RenderOpenGL::Flush()
{
	if( _iaSize )
	{
		glDrawElements(GL_TRIANGLES, _iaSize, GL_UNSIGNED_SHORT, _IndexArray);
		_vaSize = _iaSize = 0;
	}
}

MyVertex* RenderOpenGL::DrawQuad(DEV_TEXTURE tex)
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

MyVertex* RenderOpenGL::DrawFan(unsigned int nEdges)
{
	assert(nEdges*3 < INDEX_ARRAY_SIZE);

	if( _vaSize + nEdges + 1 > VERTEX_ARRAY_SIZE ||
		_iaSize + nEdges*3 > INDEX_ARRAY_SIZE )
	{
		Flush();
	}

	MyVertex *result = &_VertexArray[_vaSize];

	for( unsigned int i = 0; i < nEdges; ++i )
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

void RenderOpenGL::DrawLines(const MyLine *lines, size_t count)
{
	Flush();

	glDisable(GL_TEXTURE_2D);

	glBegin(GL_LINES);
	const MyLine *it = lines, *end = lines + count;
	for( ; it != end; ++it )
	{
		glColor4ub(it->color.rgba[3], it->color.rgba[2], it->color.rgba[1], it->color.rgba[0]);
		glVertex2fv((const float *) &it->begin);
		glVertex2fv((const float *) &it->end);
	}
	glEnd();

	SetMode(_mode); // to enable texture
}

void RenderOpenGL::SetAmbient(float ambient)
{
	_ambient = ambient;
}

//-----------------------------------------------------------------------------

std::unique_ptr<IRender> RenderCreateOpenGL()
{
	return std::unique_ptr<IRender>(new RenderOpenGL());
}

