// RenderOpenGL.cpp

#include "stdafx.h"
#include "RenderOpenGL.h"
#include "core/debug.h"

#include <gl/gl.h>


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
		HANDLE file;
		_header h;
		void *data;
	};

	///////////////////////////////////////////////////////////////////////////

	HWND   _hWnd;
	SIZE   _sizeWindow;
	RECT   _rtViewport;
	BOOL   _bDisplayChanged;

	HGLRC  _hRC;
	HDC    _hDC;
	GLuint _curtex;

	float  _ambient;

	GLushort _IndexArray[INDEX_ARRAY_SIZE];
	MyVertex _VertexArray[VERTEX_ARRAY_SIZE];

	size_t    _vaSize;      // number of filled elements in _VertexArray
	size_t    _iaSize;      // number of filled elements in _IndexArray

	RenderMode  _mode;


public:
	RenderOpenGL();

private:

	static DWORD WINAPI _ss_thread(_asyncinfo *lpInfo);

	void _cleanup();
	void Flush();


	virtual bool Init(HWND hWnd, const DisplayMode *pMode, bool bFullScreen);
	virtual void Release();

	virtual int  getModeCount() const;
	virtual bool getDisplayMode(int index, DisplayMode *pMode) const;

	virtual void OnResizeWnd();

	virtual void SetViewport(const RECT *rect);
	virtual void SetScissor(const RECT *rect);
	virtual void Camera(const RECT *vp, float x, float y, float scale, float angle);

	virtual int  GetWidth() const;
	virtual int  GetHeight() const;

	virtual int  GetViewportWidth() const;
	virtual int  GetViewportHeight() const;

	virtual void Begin(void);
	virtual void End(void);
	virtual void SetMode (const RenderMode mode);

	virtual bool TakeScreenshot(TCHAR *fileName);
	virtual void SetAmbient(float ambient);


	//
	// texture management
	//

	virtual bool TexCreate(DEV_TEXTURE &tex, Image *img);
	virtual void TexFree(DEV_TEXTURE tex);

	virtual MyVertex* DrawQuad(DEV_TEXTURE tex);
	virtual MyVertex* DrawFan(size_t nEdges);

	virtual void DrawLines(const MyLine *lines, size_t count);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

RenderOpenGL::RenderOpenGL()
{
	_bDisplayChanged = FALSE;
	_hWnd   = NULL;
	_hRC    = NULL;
	_hDC    = NULL;
	_curtex = -1;
	_vaSize = _iaSize = 0;

	ZeroMemory(_IndexArray, sizeof(GLushort) * INDEX_ARRAY_SIZE);
	ZeroMemory(_VertexArray, sizeof(MyVertex) * VERTEX_ARRAY_SIZE);
}

void RenderOpenGL::Release()
{
	_cleanup();
	delete this;
}

void RenderOpenGL::_cleanup()
{
	wglMakeCurrent(NULL, NULL);
	if( _hRC )
	{
		wglDeleteContext(_hRC);
		_hRC = NULL;
	}

	_vaSize = _iaSize = 0;

	ReleaseDC(_hWnd, _hDC);
	_hWnd = NULL;
	_hDC  = NULL;

	if( _bDisplayChanged )
	{
		ChangeDisplaySettings(NULL, 0);
		_bDisplayChanged = FALSE;
	}
}

int RenderOpenGL::getModeCount() const
{
	int count = 0;
	DEVMODE  dmode = { sizeof(DEVMODE) };
	while( EnumDisplaySettings(NULL, count, &dmode) )
	{
		++count;
	}
	return count;
}

bool RenderOpenGL::getDisplayMode(int index, DisplayMode *pMode) const
{
	DEVMODE dmode = { sizeof(DEVMODE) };
	if( BOOL res = EnumDisplaySettings(NULL, index, &dmode) )
	{
        pMode->Width        = dmode.dmPelsWidth;
		pMode->Height       = dmode.dmPelsHeight;
		pMode->BitsPerPixel = dmode.dmBitsPerPel;
		pMode->RefreshRate  = dmode.dmDisplayFrequency;
		return FALSE != res;
	}
	return false;
}

bool RenderOpenGL::Init(HWND hWnd, const DisplayMode *pMode, bool bFullScreen)
{
	TRACE("OpenGL initialization...");
	if( bFullScreen )
	{
		DEVMODE dm = {0};
		dm.dmSize             = sizeof(DEVMODE);
		dm.dmBitsPerPel       = pMode->BitsPerPixel;
		dm.dmPelsWidth        = pMode->Width;
		dm.dmPelsHeight       = pMode->Height;
		dm.dmDisplayFrequency = pMode->RefreshRate;
		dm.dmFields           = DM_BITSPERPEL | DM_PELSWIDTH |
		                        DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

		if( DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettings(&dm, CDS_FULLSCREEN) )
		{
			ChangeDisplaySettings(NULL, 0);
			return false;
		}

		_bDisplayChanged = TRUE;
	}
	else
	{
		DWORD dwStyle = GetWindowLong( hWnd, GWL_STYLE );
		dwStyle &= ~WS_POPUP;
		dwStyle |= WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX;
		SetWindowLong( hWnd, GWL_STYLE, dwStyle );

		//  Make sure our window does not hang outside of the work area
	//	SystemParametersInfo( SPI_GETWORKAREA, 0, &rcWork, 0 );
	//	GetWindowRect( hWnd, &rc );
	//	if( rc.left < rcWork.left ) rc.left = rcWork.left;
	//	if( rc.top  < rcWork.top )  rc.top  = rcWork.top;
	//	SetWindowPos( hWnd, NULL, rc.left, rc.top, 0, 0,
	//				SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
	}

	// Set window size
	RECT rc;
	SetRect( &rc, 0, 0, pMode->Width, pMode->Height );
	AdjustWindowRectEx( &rc, GetWindowLong(hWnd, GWL_STYLE),
		GetMenu(hWnd) != NULL, GetWindowLong(hWnd, GWL_EXSTYLE) );
	SetWindowPos( hWnd, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
	SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
		SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );


	_hWnd = hWnd;
	_hDC  = GetDC(hWnd);

	OnResizeWnd();


	bool result = true;

	try
	{
		PIXELFORMATDESCRIPTOR pfd ;
		ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion   = 1;
		pfd.dwFlags    = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL |
			PFD_DRAW_TO_WINDOW | PFD_GENERIC_ACCELERATED;
		pfd.iPixelType = PFD_TYPE_RGBA;

		pfd.cColorBits = GetDeviceCaps(_hDC, BITSPIXEL);
		switch( pfd.cColorBits )
		{
		case 32:
			pfd.cRedBits   = 8;
			pfd.cGreenBits = 8;
			pfd.cBlueBits  = 8;
			pfd.cAlphaBits = 8;
			break;
		default:
			throw std::runtime_error("32 bit Display required");
		}


		pfd.iLayerType = PFD_MAIN_PLANE;

		int nPixelFormat = ChoosePixelFormat(_hDC, &pfd);
		if( 0 == nPixelFormat )
			throw std::runtime_error("ChoosePixelFormat failed");

		if( !SetPixelFormat(_hDC, nPixelFormat, &pfd) )
			throw std::runtime_error("SetPixelFormat Failed");

		if( !(_hRC = wglCreateContext(_hDC)) )
			throw std::runtime_error("wglCreateContext Failed");

		if( !wglMakeCurrent(_hDC, _hRC) )
			throw std::runtime_error("wglMakeCurrent Failed");

		glEnable(GL_BLEND);

		glTexCoordPointer(2, GL_FLOAT,         sizeof(MyVertex), &_VertexArray->u    );
		glColorPointer   (4, GL_UNSIGNED_BYTE, sizeof(MyVertex), &_VertexArray->color);
		glVertexPointer  (2, GL_FLOAT,         sizeof(MyVertex), &_VertexArray->x    );

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
	}
	catch(const std::exception &e)
	{
		TRACE(" OpenGL init error: %s", e.what());
		result = false;
		_cleanup();
	}


	//
	// enable/disable vsync
	//
	typedef BOOL (APIENTRY * wglSwapIntervalEXT_Func)(int);
	wglSwapIntervalEXT_Func wglSwapIntervalEXT =
		wglSwapIntervalEXT_Func(wglGetProcAddress("wglSwapIntervalEXT"));
	if(wglSwapIntervalEXT) wglSwapIntervalEXT(0); // 1 - to enable

	SetViewport(NULL);

	return result;
}

void RenderOpenGL::OnResizeWnd()
{
	if( _hWnd )
	{
		RECT rt;
		GetClientRect(_hWnd, &rt);
		_sizeWindow.cx = rt.right - rt.left;
		_sizeWindow.cy = rt.bottom - rt.top;

		if( _hRC )
			SetViewport(NULL);
	}
}

void RenderOpenGL::SetScissor(const RECT *rect)
{
	Flush();
	if( rect )
	{
		glScissor(rect->left, _sizeWindow.cy - rect->bottom, rect->right - rect->left, rect->bottom - rect->top);
		glEnable(GL_SCISSOR_TEST);
	}
	else
	{
		glDisable(GL_SCISSOR_TEST);
	}
}

void RenderOpenGL::SetViewport(const RECT *rect)
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
			_sizeWindow.cy - rect->bottom,    // Y
			rect->right - rect->left,         // width
			rect->bottom - rect->top          // height
			);
		_rtViewport = *rect;
	}
	else
	{
		glOrtho(0, (GLdouble) _sizeWindow.cx, (GLdouble) _sizeWindow.cy, 0, -1, 1);
		glViewport(0, 0, _sizeWindow.cx, _sizeWindow.cy);
		_rtViewport.left   = 0;
		_rtViewport.top    = 0;
		_rtViewport.right  = _sizeWindow.cx;
		_rtViewport.bottom = _sizeWindow.cy;
	}
}

void RenderOpenGL::Camera(const RECT *vp, float x, float y, float scale, float angle)
{
	RenderOpenGL::SetViewport(vp);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef( (GLfloat) GetViewportWidth()/2, (GLfloat) GetViewportHeight()/2, 0 );
	glRotatef(angle * 180.0f / PI, 0,0,1);
	glTranslatef( -(GLfloat) GetViewportWidth()/2, -(GLfloat) GetViewportHeight()/2, 0 );
	glScalef(scale, scale, 1);
	glTranslatef( -x, -y, 0 );
}

int RenderOpenGL::GetWidth() const
{
	return _sizeWindow.cx;
}

int RenderOpenGL::GetHeight() const
{
	return _sizeWindow.cy;
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
	glClearColor(0, 0, 0, _ambient);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT);
}

void RenderOpenGL::End()
{
	Flush();

	glFlush();
	SwapBuffers(_hDC);
}

void RenderOpenGL::SetMode(const RenderMode mode)
{
	Flush();

	switch( mode )
	{
	case RM_LIGHT:
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
		Camera(NULL, 0, 0, 1, 0);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		break;

	default:
		assert(false);
	}

	_mode = mode;
}

bool RenderOpenGL::TexCreate(DEV_TEXTURE &tex, Image *img)
{
	glGenTextures(1, &tex.index);
	glBindTexture(GL_TEXTURE_2D, tex.index);

	glTexImage2D(
		GL_TEXTURE_2D,                      // target
		0,                                  // level
		GL_RGBA,                            // internalformat
		img->GetWidth(),                    // width
		img->GetHeight(),                   // height
		0,                                  // border
		(24==img->GetBpp())?GL_RGB:GL_RGBA, // format
		GL_UNSIGNED_BYTE,                   // type
		img->GetData()                      // pixels
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

DWORD WINAPI RenderOpenGL::_ss_thread(_asyncinfo *lpInfo)
{
	DWORD tmp;
	WriteFile(lpInfo->file, &lpInfo->h, sizeof(_header), &tmp, NULL);
	WriteFile(lpInfo->file, lpInfo->data,
		lpInfo->h.width * lpInfo->h.height * (lpInfo->h.BitPerPel / 8), &tmp, NULL);
	free(lpInfo->data);
	CloseHandle(lpInfo->file);
	delete lpInfo;
	return 0;
}

bool RenderOpenGL::TakeScreenshot(TCHAR *fileName)
{
	_asyncinfo *ai = new _asyncinfo;
	ZeroMemory(ai, sizeof(_asyncinfo));

	ai->file = CreateFile(
						fileName,
						GENERIC_WRITE,
						0,
						NULL,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
						NULL);
	if( ai->file == INVALID_HANDLE_VALUE )
	{
		delete ai;
		return false;
	}


	ai->h.DataType  = 2;  // uncompresssed
	ai->h.width     = (short) _sizeWindow.cx;
	ai->h.height    = (short) _sizeWindow.cy;
	ai->h.BitPerPel = 24;

	ai->data = malloc(ai->h.width * ai->h.height * (ai->h.BitPerPel / 8));

	if( !ai->data )
	{
		CloseHandle(ai->file);
		delete ai;
		return false;
	}

	glReadPixels(0, 0, ai->h.width, ai->h.height, GL_BGR_EXT, GL_UNSIGNED_BYTE, ai->data);

	DWORD id;
	CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE ) _ss_thread, ai, 0, &id));

	return true;
}

//-----------------------------------------------------------------------------

IRender* renderCreateOpenGL()
{
	return new RenderOpenGL();
}

///////////////////////////////////////////////////////////////////////////////
// end of file
