// RenderOpenGL.cpp

#include "RenderOpenGL.h"
#include "core/debug.h"

#include <GLFW/glfw3.h>

#define VERTEX_ARRAY_SIZE   1024
#define  INDEX_ARRAY_SIZE   2048


class RenderOpenGL : public IRender
{
	struct _header
	{
		char   IdLeight;        // text section length
		char   ColorMap;        // obsolete
		char   DataType;        // compressed or not
		char   ColorMapInfo[5]; // skip this
		short  x_origin;        // 
		short  y_origin;        // 
		short  width;           // 
		short  height;          // 
		char   BitPerPel;       // 24 or 32 only
		char   Description;     // skip this
	};

	struct _asyncinfo
	{
//		HANDLE file;
		_header h;
		void *data;
	};

	///////////////////////////////////////////////////////////////////////////

	Point  _sizeWindow;
	Rect   _rtViewport;

	GLuint _curtex;
	float  _ambient;

	GLushort _IndexArray[INDEX_ARRAY_SIZE];
	MyVertex _VertexArray[VERTEX_ARRAY_SIZE];

	size_t    _vaSize;      // number of filled elements in _VertexArray
	size_t    _iaSize;      // number of filled elements in _IndexArray

	RenderMode  _mode;


public:
	RenderOpenGL();
	virtual ~RenderOpenGL() override;

private:
	static void _ss_thread(_asyncinfo *lpInfo);

	void Flush();

	virtual void OnResizeWnd(unsigned int width, unsigned int height);

	virtual void SetViewport(const Rect *rect);
	virtual void SetScissor(const Rect *rect);
	virtual void Camera(const Rect *vp, float x, float y, float scale);

	virtual int  GetWidth() const;
	virtual int  GetHeight() const;

	virtual int  GetViewportWidth() const;
	virtual int  GetViewportHeight() const;

	virtual void Begin(void);
	virtual void End(void);
	virtual void SetMode (const RenderMode mode);

	virtual bool TakeScreenshot(char *fileName);
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
///////////////////////////////////////////////////////////////////////////////

RenderOpenGL::RenderOpenGL()
    : _sizeWindow()
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
	_sizeWindow.x = (int) width;
    _sizeWindow.y = (int) height;
    SetViewport(nullptr);
    SetScissor(nullptr);
}

void RenderOpenGL::SetScissor(const Rect *rect)
{
	Flush();
	if( rect )
	{
		glScissor(rect->left, _sizeWindow.y - rect->bottom, rect->right - rect->left, rect->bottom - rect->top);
		glEnable(GL_SCISSOR_TEST);
	}
	else
	{
		glDisable(GL_SCISSOR_TEST);
	}
}

void RenderOpenGL::SetViewport(const Rect *rect)
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
			_sizeWindow.y - rect->bottom,     // Y
			rect->right - rect->left,         // width
			rect->bottom - rect->top          // height
			);
		_rtViewport = *rect;
	}
	else
	{
		glOrtho(0, (GLdouble) _sizeWindow.x, (GLdouble) _sizeWindow.y, 0, -1, 1);
		glViewport(0, 0, _sizeWindow.x, _sizeWindow.y);
		_rtViewport.left   = 0;
		_rtViewport.top    = 0;
		_rtViewport.right  = _sizeWindow.x;
		_rtViewport.bottom = _sizeWindow.y;
	}
}

void RenderOpenGL::Camera(const Rect *vp, float x, float y, float scale)
{
	SetViewport(vp);
	SetScissor(vp);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(scale, scale, 1);
	if (vp)
		glTranslatef((float) WIDTH(*vp) / 2 / scale - x, (float) HEIGHT(*vp) / 2 / scale - y, 0);
	else
		glTranslatef(0, 0, 0);
}

int RenderOpenGL::GetWidth() const
{
	return _sizeWindow.x;
}

int RenderOpenGL::GetHeight() const
{
	return _sizeWindow.y;
}

int RenderOpenGL::GetViewportWidth() const
{
	return _rtViewport.right - _rtViewport.left;
}

int RenderOpenGL::GetViewportHeight() const
{
	return _rtViewport.bottom - _rtViewport.top;
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

MyVertex* RenderOpenGL::DrawFan(size_t nEdges)
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

void RenderOpenGL::_ss_thread(_asyncinfo *lpInfo)
{
//	DWORD tmp;
//	WriteFile(lpInfo->file, &lpInfo->h, sizeof(_header), &tmp, NULL);
//	WriteFile(lpInfo->file, lpInfo->data,
//		lpInfo->h.width * lpInfo->h.height * (lpInfo->h.BitPerPel / 8), &tmp, NULL);
	free(lpInfo->data);
//	CloseHandle(lpInfo->file);
	delete lpInfo;
}

bool RenderOpenGL::TakeScreenshot(char *fileName)
{
	_asyncinfo *ai = new _asyncinfo;
	memset(ai, 0, sizeof(_asyncinfo));

//	ai->file = CreateFile(
//						fileName,
//						GENERIC_WRITE,
//						0,
//						NULL,
//						CREATE_ALWAYS,
//						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
//						NULL);
//	if( ai->file == INVALID_HANDLE_VALUE )
//	{
//		delete ai;
//		return false;
//	}


	ai->h.DataType  = 2;  // uncompresssed
	ai->h.width     = (short) _sizeWindow.x;
	ai->h.height    = (short) _sizeWindow.y;
	ai->h.BitPerPel = 24;

	ai->data = malloc(ai->h.width * ai->h.height * (ai->h.BitPerPel / 8));

	if( !ai->data )
	{
//		CloseHandle(ai->file);
		delete ai;
		return false;
	}

//	glReadPixels(0, 0, ai->h.width, ai->h.height, GL_BGR_EXT, GL_UNSIGNED_BYTE, ai->data);

//	DWORD id;
//	CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE ) _ss_thread, ai, 0, &id));

	return true;
}

//-----------------------------------------------------------------------------

std::unique_ptr<IRender> RenderCreateOpenGL()
{
	return std::unique_ptr<IRender>(new RenderOpenGL());
}

///////////////////////////////////////////////////////////////////////////////
// end of file
