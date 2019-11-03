#include "inc/video/RenderOpenGL.h"
#include <GLFW/glfw3.h>
#include <cstring>

#define VERTEX_ARRAY_SIZE   1024
#define  INDEX_ARRAY_SIZE   2048


class RenderOpenGL final
	: public IRender
{
public:
	RenderOpenGL();
	~RenderOpenGL() override;

private:
	void Flush();

	void SetScissor(const RectRB &rect) override;
	void SetViewport(const RectRB &rect) override;
	void SetTransform(vec2d offset, float scale) override;

	void Begin(unsigned int displayWidth, unsigned int displayHeight, DisplayOrientation displayOrientation) override;
	void End() override;
	void SetMode(const RenderMode mode) override;

	void SetAmbient(float ambient) override;


	//
	// texture management
	//

	bool TexCreate(DEV_TEXTURE &tex, ImageView img, bool magFilter) override;
	void TexFree(DEV_TEXTURE tex) override;

	MyVertex* DrawQuad(DEV_TEXTURE tex) override;
	MyVertex* DrawFan(unsigned int nEdges) override;

	void DrawLines(const MyLine *lines, size_t count) override;

	GLint _windowHeight = 0;
	GLuint _curtex = -1;
	float  _ambient = 0;

	GLushort _IndexArray[INDEX_ARRAY_SIZE];
	MyVertex _VertexArray[VERTEX_ARRAY_SIZE];

	unsigned int _vaSize = 0;      // number of filled elements in _VertexArray
	unsigned int _iaSize = 0;      // number of filled elements in _IndexArray

	RenderMode  _mode;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

RenderOpenGL::RenderOpenGL()
	: _curtex(-1)
	, _vaSize(0)
	, _iaSize(0)
{
	memset(_IndexArray, 0, sizeof(GLushort) * INDEX_ARRAY_SIZE);
	memset(_VertexArray, 0, sizeof(MyVertex) * VERTEX_ARRAY_SIZE);
}

RenderOpenGL::~RenderOpenGL()
{
}

void RenderOpenGL::SetScissor(const RectRB &rect)
{
	Flush();
	glScissor(rect.left, _windowHeight - rect.bottom, WIDTH(rect), HEIGHT(rect));
	glEnable(GL_SCISSOR_TEST);
}

void RenderOpenGL::SetViewport(const RectRB &rect)
{
	Flush();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, (GLdouble) WIDTH(rect), (GLdouble) HEIGHT(rect), 0, -1, 1);
	glViewport(
		rect.left,                      // X
		_windowHeight - rect.bottom,    // Y
		WIDTH(rect),          // width
		HEIGHT(rect)          // height
		);
}

void RenderOpenGL::SetTransform(vec2d offset, float scale)
{
	Flush();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(offset.x, offset.y, 0);
	glScalef(scale, scale, 1);
}

void RenderOpenGL::Begin(unsigned int displayWidth, unsigned int displayHeight, DisplayOrientation displayOrientation)
{
	_windowHeight = displayHeight;

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
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
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
//		Camera(nullptr, 0, 0, 1);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		break;

	default:
		assert(false);
	}

	_mode = mode;
}

bool RenderOpenGL::TexCreate(DEV_TEXTURE &tex, ImageView img, bool magFilter)
{
    assert(img.stride == img.width * img.bpp / 8);
	glGenTextures(1, &tex.index);
	glBindTexture(GL_TEXTURE_2D, tex.index);

	glTexImage2D(
		GL_TEXTURE_2D,                      // target
		0,                                  // level
		GL_RGBA,                            // internalformat
		img.width,                          // width
		img.height,                         // height
		0,                                  // border
		(24==img.bpp) ? GL_RGB : GL_RGBA,   // format
		GL_UNSIGNED_BYTE,                   // type
		img.pixels                          // pixels
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter ? GL_LINEAR : GL_NEAREST);
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
		_vaSize = 0;
		_iaSize = 0;
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

