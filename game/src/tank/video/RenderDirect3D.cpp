// RenderDirect3D.cpp

#include "stdafx.h"
#include "RenderDirect3D.h"

#include "macros.h"
#include "core/debug.h"
#include "core/ComPtr.h"

#include <d3d9.h>
#include <d3dx9math.h>


#pragma comment(lib, "d3dx9.lib")


#define VERTEX_BUFFER_SIZE   1024
#define  INDEX_BUFFER_SIZE   2048


#if 0
#define MYVERTEX_FORMAT (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1|D3DFVF_TEX2)
#else
#define MYVERTEX_FORMAT (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#endif


#ifdef _DEBUG
#define V(x) assert(SUCCEEDED(x))
#else
#define V(x) x
#endif


class Direct3D_Exception
{
public:
	HRESULT hr;
	explicit Direct3D_Exception(HRESULT hr_) { hr = hr_; }
};


class RenderDirect3D : public IRender
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

	ComPtr<IDirect3D9>              _d3D;
	ComPtr<IDirect3DDevice9>        _pd3dDevice;
	ComPtr<IDirect3DVertexBuffer9>  _pVB;
	ComPtr<IDirect3DIndexBuffer9>   _pIB;

	IDirect3DTexture9 *_curtex;


	HWND   _hWnd;
	SIZE   _sizeWindow;
	RECT   _rtViewport;

	float  _ambient;

	WORD       *_indexArray;
	MyVertex  *_VertexArray;

	size_t    _vaSize;      // number of filled elements in _VertexArray
	size_t    _iaSize;      // number of filled elements in _indexArray

	RenderMode  _mode;


public:
	RenderDirect3D();

private:

	static DWORD WINAPI _ss_thread(_asyncinfo *lpInfo);

	void _cleanup();
	void _flush();

	virtual bool Init(HWND hWnd, const DisplayMode *pMode, bool bFullScreen);
	virtual void Release();

	virtual int  getModeCount() const;
	virtual bool getDisplayMode(int index, DisplayMode *pMode) const;

	virtual void OnResizeWnd();

	virtual void SetScissor(const RECT *rect);
	virtual void SetViewport(const RECT *rect);
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

	virtual void DrawLines(const MyLine *lines, size_t count) {}; // TODO: implement
};

///////////////////////////////////////////////////////////////////////////////

static void MatrixOrthoOffCenterRH(D3DXMATRIX &m, float l, float r, float b, float t)
{
//	D3DXMatrixOrthoOffCenterRH(&m, r, b, l, t, 1, -1);
	float zn =  1;
	float zf = -1;
	m._11 = 2/(r-l);      m._12 = 0;            m._13 = 0;           m._14 = 0;
	m._21 = 0;            m._22 = 2/(t-b);      m._23 = 0;           m._24 = 0;
	m._31 = 0;            m._32 = 0;            m._33 = 1/(zn-zf);   m._34 = 0;
	m._41 = (l+r)/(l-r);  m._42 = (t+b)/(b-t);  m._43 = zn/(zn-zf);  m._44 = 1;
}

///////////////////////////////////////////////////////////////////////////////

typedef IDirect3D9* (__stdcall *D3DCREATEPROC)(UINT);


RenderDirect3D::RenderDirect3D()
{
    _hWnd        = NULL;

	_vaSize      = 0;
	_iaSize      = 0;

	_indexArray  = NULL;
	_VertexArray = NULL;

	_curtex      = NULL;

	//--------------------------------------

	HMODULE hmod = LoadLibrary("d3d9.dll");
	if( NULL == hmod )
		throw Direct3D_Exception(E_FAIL);

	D3DCREATEPROC create = (D3DCREATEPROC) GetProcAddress(hmod, "Direct3DCreate9");
	if( NULL == create )
		throw Direct3D_Exception(E_FAIL);

	if( NULL == (_d3D = create(D3D_SDK_VERSION)) )
		throw Direct3D_Exception(E_FAIL);
}

void RenderDirect3D::Release()
{
	_cleanup();
	delete this;
}

void RenderDirect3D::_cleanup()
{
	_curtex = NULL;

	if( _indexArray )
	{
		_pIB->Unlock();
		_indexArray = NULL;
	}

	if( _VertexArray )
	{
		_pVB->Unlock();
		_VertexArray = NULL;
	}

	_pIB.Release();
	_pVB.Release();
	_pd3dDevice.Release();
	_d3D.Release();
}

int RenderDirect3D::getModeCount() const
{
	assert(_d3D);
	return _d3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
}

bool RenderDirect3D::getDisplayMode(int index, DisplayMode *pMode) const
{
	assert(_d3D);

	D3DDISPLAYMODE mode;

	if( FAILED(_d3D->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, index, &mode)) )
		return false;

	pMode->BitsPerPixel = 32;
	pMode->Width        = mode.Width;
	pMode->Height       = mode.Height;
	pMode->RefreshRate  = mode.RefreshRate;

	return true;
}

bool RenderDirect3D::Init(HWND hWnd, const DisplayMode *pMode, bool bFullScreen)
{
	//
	// resize window
	//

	RECT rc;

	DWORD dwStyle = GetWindowLong( hWnd, GWL_STYLE );
	dwStyle &= ~(WS_POPUP|WS_CAPTION);
	if( !bFullScreen )
		dwStyle |= WS_CAPTION;
	dwStyle |= WS_OVERLAPPED | WS_THICKFRAME | WS_MINIMIZEBOX;
	SetWindowLong( hWnd, GWL_STYLE, dwStyle );

	// Set window size
	SetRect( &rc, 0, 0, pMode->Width, pMode->Height );

	AdjustWindowRectEx( &rc, GetWindowLong(hWnd, GWL_STYLE),
		GetMenu(hWnd) != NULL, GetWindowLong(hWnd, GWL_EXSTYLE) );

	SetWindowPos( hWnd, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
				SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

	SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
				SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );

	_hWnd = hWnd;
	OnResizeWnd();


	//
	// create device
	//

	D3DPRESENT_PARAMETERS params = {0};
	params.BackBufferWidth  = pMode->Width;
	params.BackBufferHeight = pMode->Height;
	params.BackBufferFormat = D3DFMT_A8R8G8B8;

	params.MultiSampleType              = D3DMULTISAMPLE_NONE;
	params.MultiSampleQuality           = 0;
	params.SwapEffect                   = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow                = hWnd;
	params.Windowed                     = !bFullScreen;
	params.EnableAutoDepthStencil       = FALSE;
	params.AutoDepthStencilFormat       = D3DFMT_UNKNOWN;
	params.Flags                        = 0;
	params.FullScreen_RefreshRateInHz   = bFullScreen?pMode->RefreshRate:0;
    params.PresentationInterval         = D3DPRESENT_INTERVAL_IMMEDIATE;

	if( FAILED(_d3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE, &params, &_pd3dDevice)) )
	{
		TRACE("Hardware vertex processing not supported; trying software");
		if( FAILED(_d3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE, &params, &_pd3dDevice)) )
		{
			TRACE("ERROR: CreateDevice failed")
			return false;
		}
	}

	//
	// setup device state
	//

	// Turn off culling, so we see the front and back of the triangle
	V(_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ));

	// Turn off D3D lighting, since we are providing our own vertex colors
	V(_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE ));

	// Turn off the zbuffer
	V(_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE ));


	V(_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE ));

	V(_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE ));
	V(_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE ));
	V(_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE ));
	V(_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE ));

	V(_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	V(_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));

	V(_pd3dDevice->SetFVF(MYVERTEX_FORMAT));


	//
	// create vertex and index biffers
	//

	if( FAILED(_pd3dDevice->CreateVertexBuffer(sizeof(MyVertex)*VERTEX_BUFFER_SIZE,
		D3DUSAGE_WRITEONLY, MYVERTEX_FORMAT, D3DPOOL_DEFAULT, &_pVB, NULL)) )
	{
		TRACE("ERROR: CreateVertexBuffer failed")
		return false;
	}

	if( FAILED(_pd3dDevice->CreateIndexBuffer(sizeof(WORD)*INDEX_BUFFER_SIZE,
		D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &_pIB, NULL)) )
	{
		TRACE("ERROR: CreateIndexBuffer failed")
		return false;
	}

	V(_pd3dDevice->SetStreamSource( 0, _pVB, 0, sizeof(MyVertex)));
	V(_pd3dDevice->SetIndices(_pIB));

	V(_pVB->Lock(0,sizeof(MyVertex)*VERTEX_BUFFER_SIZE,(void**)&_VertexArray,D3DLOCK_DISCARD));
	V(_pIB->Lock(0,sizeof(WORD)*INDEX_BUFFER_SIZE,(void**)&_indexArray, NULL));

	ZeroMemory(_VertexArray, sizeof(MyVertex)*VERTEX_BUFFER_SIZE);

	SetViewport(NULL);

	return true;
}

void RenderDirect3D::OnResizeWnd()
{
	if( _hWnd )
	{
		RECT rt;
		GetClientRect( _hWnd, &rt );
		_sizeWindow.cx = rt.right - rt.left;
		_sizeWindow.cy = rt.bottom - rt.top;

		if( _pd3dDevice )
			SetViewport(NULL);
	}
}

void RenderDirect3D::SetScissor(const RECT *rect)
{
	if( _iaSize )
		_flush();

	if( rect )
	{
		_pd3dDevice->SetScissorRect(rect);
		_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
	}
	else
	{
		_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	}
}

void RenderDirect3D::SetViewport(const RECT *rect)
{
	if( _iaSize ) _flush();

	D3DXMATRIX m;
	D3DVIEWPORT9 vp;
	vp.MinZ = 0;
	vp.MaxZ = 1;

	if( rect )
	{
		MatrixOrthoOffCenterRH(m, 0, (float) (rect->right - rect->left),
			(float) (rect->bottom - rect->top), 0);

		vp.X = rect->left;
		vp.Y = _sizeWindow.cy - rect->bottom;
		vp.Width  = rect->right - rect->left;
		vp.Height = rect->bottom - rect->top;

		V(_pd3dDevice->SetTransform(D3DTS_PROJECTION, &m));
		V(_pd3dDevice->SetViewport(&vp));

		_rtViewport = *rect;
	}
	else
	{
		MatrixOrthoOffCenterRH(m, 0, (float) _sizeWindow.cx, (float) _sizeWindow.cy, 0);
		V(_pd3dDevice->SetTransform(D3DTS_PROJECTION, &m));

		vp.X = 0;
		vp.Y = 0;
		vp.Width  = _sizeWindow.cx;
		vp.Height = _sizeWindow.cy;
		V(_pd3dDevice->SetViewport(&vp));

		_rtViewport.left   = 0;
		_rtViewport.top    = 0;
		_rtViewport.right  = _sizeWindow.cx;
		_rtViewport.bottom = _sizeWindow.cy;
	}
}

void RenderDirect3D::Camera(const RECT *vp, float x, float y, float scale, float angle)
{
	RenderDirect3D::SetViewport(vp);

	D3DXMATRIX m;

	MatrixOrthoOffCenterRH(m, 0, (float) GetViewportWidth(), (float) GetViewportHeight(), 0);
	V(_pd3dDevice->SetTransform(D3DTS_PROJECTION, &m));

//	D3DXMatrixScaling(&m, scale, scale, 1.0f);
	m._11 = m._22 = scale;
	m._33 = m._44 = 1;
	m._12 = m._13 = m._14 = m._21 = 0;
	m._23 = m._24 = m._31 = m._32 = 0;
	m._34 = m._41 = m._42 = m._43 = 0;
	V(_pd3dDevice->SetTransform(D3DTS_VIEW, &m));

//	D3DXMatrixTranslation(&m, -x - 0.5f, -y - 0.5f, 0);
	m._41 = -x - 0.5f;
	m._42 = -y - 0.5f;
	m._12 = m._13 = m._14 = m._21 = m._23 = m._24 = m._31 = m._32 = m._34 = m._43 = 0;
	m._11 = m._22 = m._33 = m._44 = 1;
	V(_pd3dDevice->SetTransform(D3DTS_WORLD, &m));

}

int RenderDirect3D::GetWidth() const
{
	return _sizeWindow.cx;
}

int RenderDirect3D::GetHeight() const
{
	return _sizeWindow.cy;
}

int RenderDirect3D::GetViewportWidth() const
{
	return _rtViewport.right - _rtViewport.left;
}

int RenderDirect3D::GetViewportHeight() const
{
	return _rtViewport.bottom - _rtViewport.top;
}

void RenderDirect3D::Begin()
{
	SpriteColor color;
	color.dwColor = 0x00000000;
	color.a = (BYTE) __max(0, __min(255, int(255.0f * _ambient)));

	V(_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, color.dwColor, 0, 0));
	V(_pd3dDevice->BeginScene());
}

void RenderDirect3D::End()
{
	if( _iaSize ) _flush();

	V(_pd3dDevice->EndScene());
	V(_pd3dDevice->Present(NULL, NULL, NULL, NULL));
}

void RenderDirect3D::SetMode(const RenderMode mode)
{
	if( _iaSize ) _flush();

    switch(mode)
	{
	case RM_LIGHT:
		_curtex = NULL;
		V(_pd3dDevice->SetTexture( 0, NULL ));
		V(_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA  ));
		V(_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE       ));
		V(_pd3dDevice->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA));
		break;

	case RM_WORLD:
		V(_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_DESTALPHA   ));
		V(_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA ));
		V(_pd3dDevice->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED));
		break;

	case RM_INTERFACE:
		SetScissor(NULL);
		Camera(NULL, 0, 0, 1, 0);
		V(_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE         ));
		V(_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA ));
		V(_pd3dDevice->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED));
		break;

	default:
		assert(false);
	}

	_mode = mode;
}

bool RenderDirect3D::TexCreate(DEV_TEXTURE &tex, Image *img)
{
	IDirect3DTexture9 *pTex = NULL;
	HRESULT hr;

	hr = _pd3dDevice->CreateTexture(
		img->GetWidth(), // Width
		img->GetHeight(),// Height
		1,               // Levels
		0,               // Usage
		D3DFMT_A8R8G8B8, // Format
		D3DPOOL_MANAGED, // Pool    ?D3DPOOL_DEFAULT?
		&pTex,           // ppTexture
		NULL             // pSharedHandle.
	);

    if( FAILED(hr) )
		return false;

	D3DLOCKED_RECT lr;
	if( FAILED(hr = pTex->LockRect(0, &lr, NULL, D3DLOCK_DISCARD)) )
	{
		pTex->Release();
		return false;
	}

	size_t size = img->GetWidth() * img->GetHeight() * 4;
	unsigned char *src = (unsigned char *) img->GetData();
	unsigned char *dst = (unsigned char *) lr.pBits;
	unsigned char *end = dst + size;
	bool bpp24 = (24 == img->GetBpp());
	while( dst != end )
	{
		*(dst++) = src[2];
		*(dst++) = src[1];
		*(dst++) = src[0];
		if( bpp24 )
		{
			*(dst++) = 255;
			src += 3;
		}
		else
		{
			*(dst++) = src[3];
			src += 4;
		}
	}

	V(pTex->UnlockRect(0));

	tex.ptr = pTex;
	return true;
}

void RenderDirect3D::TexFree(DEV_TEXTURE tex)
{
	if( _curtex == tex.ptr )
	{
		_curtex = NULL;
		V(_pd3dDevice->SetTexture(0, NULL));
	}
	((IDirect3DTexture9 *) tex.ptr)->Release();
}

void RenderDirect3D::_flush()
{
	assert(_VertexArray);
	assert(_indexArray);

	V(_pVB->Unlock());
	V(_pIB->Unlock());

	V(_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, _vaSize, 0, _iaSize/3));

	V(_pVB->Lock(0, sizeof(MyVertex)*VERTEX_BUFFER_SIZE,
		(void**) &_VertexArray, D3DLOCK_DISCARD));
	V(_pIB->Lock(0, sizeof(WORD)*INDEX_BUFFER_SIZE, (void**) &_indexArray, NULL));

	ZeroMemory(_VertexArray, sizeof(MyVertex)*VERTEX_BUFFER_SIZE);

	_vaSize = _iaSize = 0;
}

MyVertex* RenderDirect3D::DrawQuad(DEV_TEXTURE tex)
{
	if( _curtex != tex.ptr )
	{
		if( _iaSize ) _flush();
		_curtex = ((IDirect3DTexture9 *) tex.ptr);
		V(_pd3dDevice->SetTexture(0, _curtex));
	}
	if( _vaSize > VERTEX_BUFFER_SIZE - 4 || _iaSize > INDEX_BUFFER_SIZE  - 6 )
	{
		_flush();
	}

	MyVertex *result = _VertexArray + _vaSize;

	_indexArray[_iaSize]   = _vaSize;
	_indexArray[_iaSize+1] = _vaSize+1;
	_indexArray[_iaSize+2] = _vaSize+2;
	_indexArray[_iaSize+3] = _vaSize;
	_indexArray[_iaSize+4] = _vaSize+2;
	_indexArray[_iaSize+5] = _vaSize+3;

	_iaSize += 6;
	_vaSize += 4;

	return result;
}

MyVertex* RenderDirect3D::DrawFan(size_t nEdges)
{
	assert(nEdges*3 < INDEX_BUFFER_SIZE);

	if( _vaSize + nEdges   > VERTEX_BUFFER_SIZE - 1 ||
		_iaSize + nEdges*3 > INDEX_BUFFER_SIZE )
	{
		_flush();
	}

	MyVertex *result = &_VertexArray[_vaSize];

	for( size_t i = 0; i < nEdges; ++i )
	{
		_indexArray[_iaSize + i*3    ] = _vaSize;
		_indexArray[_iaSize + i*3 + 1] = _vaSize + i + 1;
		_indexArray[_iaSize + i*3 + 2] = _vaSize + i + 2;
	}
	_indexArray[_iaSize + nEdges*3 - 1] = _vaSize + 1;

	_iaSize += nEdges*3;
	_vaSize += nEdges+1;

	return result;
}

void RenderDirect3D::SetAmbient(float ambient)
{
	_ambient = ambient;
}

DWORD WINAPI RenderDirect3D::_ss_thread(_asyncinfo *lpInfo)
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

bool RenderDirect3D::TakeScreenshot(TCHAR *fileName)
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


	ai->h.DataType  = 2;  // uncompressed
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

  //  glReadPixels(0, 0, ai->h.width, ai->h.height, GL_BGR_EXT, GL_UNSIGNED_BYTE, ai->data);

	DWORD id;
	CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE ) _ss_thread, ai, 0, &id));

	return true;
}

//-----------------------------------------------------------------------------

IRender* renderCreateDirect3D()
{
	return new RenderDirect3D();
}

///////////////////////////////////////////////////////////////////////////////
// end of file
