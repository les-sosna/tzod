// RenderDirect3D.cpp

#include "stdafx.h"
#include "RenderDirect3D.h"

#include "macros.h"

#include "gc/GameClasses.h"
#include "gc/RigidBody.h"


#include <d3d9.h>
#include <d3dx9math.h>


#define VERTEX_BUFFER_SIZE   1024
#define  INDEX_BUFFER_SIZE   2048


#if 0
#define MYVERTEX_FORMAT (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1|D3DFVF_TEX2)
#else
#define MYVERTEX_FORMAT (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#endif


#ifdef _DEBUG
#define V(x) _ASSERT(SUCCEEDED(x))
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
		char   IdLeight;	//   Длина текстовой информации после первого
		char   ColorMap;	//   Идентификатор наличия цветовой карты - устарел
		char   DataType;	//   Тип данных - запакованный или нет
		char   ColorMapInfo[5];	//   Информация о цветовой карте - нужно пропустить эти 5 байт
		short  x_origin;	//   Начало изображения по оси X
		short  y_origin;	//   Начало изображения по оси Y
		short  width;		//   Ширина изображения
		short  height;		//   Высота изображения
		char   BitPerPel;	//   Кол-во бит на пиксель - здесь только 24 или 32
		char   Description;	//   Описание - пропускайте
	};

	struct _asyncinfo
	{
		HANDLE file;
		_header h;
        void *data;
	};

	IDirect3D9              *_d3D;
	IDirect3DDevice9        *_pd3dDevice;
	IDirect3DVertexBuffer9  *_pVB;
	IDirect3DIndexBuffer9   *_pIB;
	IDirect3DTexture9       *_curtex;


    HWND   _hWnd;
    SIZE   _sizeWindow;
	RECT   _rtViewport;

	float  _ambient;

	WORD       *_IndexArray;
	MyVertex  *_VertexArray;

	size_t    _vaSize;      // number of filled elements in _VertexArray
	size_t    _iaSize;      // number of filled elements in _IndexArray

	RENDER_MODE  _mode;


public:
	RenderDirect3D();

private:

	static DWORD WINAPI _ss_thread(_asyncinfo *lpInfo);

	void _cleanup();
	void _flush();

	virtual BOOL Init(HWND hWnd, const DisplayMode *pMode, BOOL bFullScreen);
	virtual void Release();

	virtual DWORD getModeCount() const;
	virtual BOOL  getDisplayMode(DWORD index, DisplayMode *pMode) const;

	virtual void OnResizeWnd();

	virtual void SetViewport(const RECT *rect);
	virtual void Camera(float x, float y, float scale, float angle);

	virtual int  getXsize() const;
    virtual int  getYsize() const;

    virtual int  getViewportXsize() const;
    virtual int  getViewportYsize() const;

	virtual void Begin(void);
	virtual void End(void);
	virtual void setMode (const RENDER_MODE mode);

	virtual bool TakeScreenshot(TCHAR *fileName);
	virtual void SetAmbient(float ambient);


	//
	// texture management
	//

	virtual bool TexCreate(DEV_TEXTURE &tex, const TextureData *pData);
	virtual void TexFree(DEV_TEXTURE tex);
	virtual void TexBind(DEV_TEXTURE tex);

	virtual MyVertex* DrawQuad();
	virtual MyVertex* DrawFan(size_t nEdges);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef IDirect3D9* (__stdcall *D3DCREATEPROC)(UINT);


RenderDirect3D::RenderDirect3D()
{
	_d3D         = NULL;
	_pd3dDevice  = NULL;
	_pVB         = NULL;
	_pIB         = NULL;

    _hWnd        = NULL;

	_vaSize      = 0;
	_iaSize      = 0;

	_IndexArray  = NULL;
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

	if( _IndexArray )
	{
		_pIB->Unlock();
		_IndexArray = NULL;
	}

	if( _VertexArray )
	{
		_pVB->Unlock();
		_VertexArray = NULL;
	}

	SAFE_RELEASE(_pIB);
	SAFE_RELEASE(_pVB);
	SAFE_RELEASE(_pd3dDevice);
	SAFE_RELEASE(_d3D);
}

DWORD RenderDirect3D::getModeCount() const
{
	_ASSERT(_d3D);
	return _d3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
}

BOOL RenderDirect3D::getDisplayMode(DWORD index, DisplayMode *pMode) const
{
	_ASSERT(_d3D);

	D3DDISPLAYMODE mode;

	if( FAILED(_d3D->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, index, &mode)) )
		return FALSE;

	pMode->BitsPerPixel = 32;
	pMode->Width        = mode.Width;
	pMode->Height       = mode.Height;
	pMode->RefreshRate  = mode.RefreshRate;

	return TRUE;
}

BOOL RenderDirect3D::Init(HWND hWnd, const DisplayMode *pMode, BOOL bFullScreen)
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
//    params.PresentationInterval         = D3DPRESENT_INTERVAL_IMMEDIATE;

	if( FAILED(_d3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &params, &_pd3dDevice)) )
	{
		if( FAILED(_d3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING, &params, &_pd3dDevice)) )
		{
			return FALSE;
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
		return FALSE;
	}

	if( FAILED(_pd3dDevice->CreateIndexBuffer(sizeof(WORD)*INDEX_BUFFER_SIZE,
		D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &_pIB, NULL)) )
	{
		return FALSE;
	}

	V(_pd3dDevice->SetStreamSource( 0, _pVB, 0, sizeof(MyVertex)));
	V(_pd3dDevice->SetIndices(_pIB));


	V(_pVB->Lock(0,sizeof(MyVertex)*VERTEX_BUFFER_SIZE,(void**)&_VertexArray,D3DLOCK_DISCARD));
	V(_pIB->Lock(0,sizeof(WORD)*INDEX_BUFFER_SIZE,(void**)&_IndexArray, NULL));

	ZeroMemory(_VertexArray, sizeof(MyVertex)*VERTEX_BUFFER_SIZE);

	return TRUE;
}

void RenderDirect3D::OnResizeWnd()
{
    if( !_hWnd ) return;

	RECT rt;
	GetClientRect( _hWnd, &rt );
	_sizeWindow.cx = rt.right - rt.left;
	_sizeWindow.cy = rt.bottom - rt.top;
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
		if( RM_INTERFACE == _mode )
		{
			D3DXMatrixOrthoOffCenterRH(&m, (float) rect->left, (float) rect->right,
				(float) rect->bottom, (float) rect->top, 1, -1);

			vp.X = rect->left;
			vp.Y = rect->top;
			vp.Width  = rect->right - rect->left;
			vp.Height = rect->bottom - rect->top;
		}
		else
		{
			D3DXMatrixOrthoOffCenterRH(&m, 0, (float) (rect->right - rect->left),
				(float) (rect->bottom - rect->top), 0, 1, -1);

			vp.X = rect->left;
			vp.Y = _sizeWindow.cy - rect->bottom;
			vp.Width  = rect->right - rect->left;
			vp.Height = rect->bottom - rect->top;
		}


		V(_pd3dDevice->SetTransform(D3DTS_PROJECTION, &m));
		V(_pd3dDevice->SetViewport(&vp));

		_rtViewport = *rect;
	}
	else
	{
		D3DXMatrixOrthoOffCenterRH(&m, 0, (float) _sizeWindow.cx, (float) _sizeWindow.cy, 0, 1, -1);
		V(_pd3dDevice->SetTransform(D3DTS_PROJECTION, &m));

		vp.X = 0;
		vp.Y = 0;
		vp.Width  = _sizeWindow.cx;
		vp.Height = _sizeWindow.cy;
		V(_pd3dDevice->SetViewport(&vp));

		_rtViewport.left   = _rtViewport.top = 0;
		_rtViewport.right  = _sizeWindow.cx;
		_rtViewport.bottom = _sizeWindow.cy;
	}
}

void RenderDirect3D::Camera(float x, float y, float scale, float angle)
{
	if( _iaSize ) _flush();

	D3DXMATRIX m;

//                                 l                  r                   b                   t
	D3DXMatrixOrthoOffCenterRH(&m, 0, (float) getViewportXsize(), (float) getViewportYsize(), 0, 1, -1);

/*	float l  =  0;
	float r  =  (float) getViewportXsize();
	float b  =  (float) getViewportYsize();
	float t  =  0;
	float zn =  1;
	float zf = -1;

	m._11 = 2/(r-l);      m._12 = 0;            m._13 = 0;           m._14 = 0;
	m._21 = 0;            m._22 = 2/(t-b);      m._23 = 0;           m._24 = 0;
	m._31 = 0;            m._32 = 0;            m._33 = 1/(zn-zf);   m._34 = 0;
	m._41 = (l+r)/(l-r);  m._42 = (t+b)/(b-t);  m._43 = zn/(zn-zf);  m._44 = l;
*/

	V(_pd3dDevice->SetTransform(D3DTS_PROJECTION, &m));

	D3DXMatrixScaling(&m, scale, scale, 1.0f);
	V(_pd3dDevice->SetTransform(D3DTS_VIEW, &m));

	D3DXMatrixTranslation(&m, -x - 0.5f, -y - 0.5f, 0);
	V(_pd3dDevice->SetTransform(D3DTS_WORLD, &m));

}

int RenderDirect3D::getXsize() const
{
	return _sizeWindow.cx;
}

int RenderDirect3D::getYsize() const
{
	return _sizeWindow.cy;
}

int RenderDirect3D::getViewportXsize() const
{
	return _rtViewport.right - _rtViewport.left;
}

int RenderDirect3D::getViewportYsize() const
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

void RenderDirect3D::setMode(const RENDER_MODE mode)
{
	if( _iaSize ) _flush();

    switch(mode)
	{
	case RM_LIGHT:
		V(_pd3dDevice->SetTexture( 0, NULL )); _curtex = NULL;
		V(_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA  ));
		V(_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE       ));
		break;

	case RM_WORLD:
		V(_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_DESTALPHA   ));
		V(_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA ));
		break;

	case RM_INTERFACE:
		SetViewport(NULL);
		Camera(0, 0, 1, 0);
		V(_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE         ));
		V(_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA ));
		break;

	default:
        _ASSERT(FALSE);
	}

	_mode = mode;
}

bool RenderDirect3D::TexCreate(DEV_TEXTURE &tex, const TextureData *pData)
{
	IDirect3DTexture9 *pTex = NULL;
	HRESULT hr;

	hr = _pd3dDevice->CreateTexture(
		pData->width,    // Width
		pData->height,   // Height
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

	size_t size = pData->width*pData->height*4;
	unsigned char *src = (unsigned char *) pData->imageData;
	unsigned char *dst = (unsigned char *) lr.pBits;
	unsigned char *end = dst + pData->width*pData->height*4;
    while( dst != end )
	{
		*(dst++) = src[2];
		*(dst++) = src[1];
		*(dst++) = src[0];
		if( 24==pData->bpp )
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
	if( _curtex == tex.ptr ) _curtex = NULL;
	((IDirect3DTexture9 *) tex.ptr)->Release();
}

void RenderDirect3D::TexBind(DEV_TEXTURE tex)
{
	if( _curtex == tex.ptr ) return;
	if( _iaSize ) _flush();
	_curtex = ((IDirect3DTexture9 *) tex.ptr);
	V(_pd3dDevice->SetTexture(0, _curtex));
}

void RenderDirect3D::_flush()
{
//	_FpsCounter::Inst()->OneMoreBatch();

	_ASSERT(_VertexArray);
	_ASSERT(_IndexArray);

	V(_pVB->Unlock());
	V(_pIB->Unlock());

	V(_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, _vaSize, 0, _iaSize/3));

	V(_pVB->Lock(0, sizeof(MyVertex)*VERTEX_BUFFER_SIZE,
		(void**) &_VertexArray, D3DLOCK_DISCARD));
	V(_pIB->Lock(0, sizeof(WORD)*INDEX_BUFFER_SIZE, (void**) &_IndexArray, NULL));

	ZeroMemory(_VertexArray, sizeof(MyVertex)*VERTEX_BUFFER_SIZE);

	_vaSize = _iaSize = 0;
}

MyVertex* RenderDirect3D::DrawQuad()
{
	if( _vaSize > VERTEX_BUFFER_SIZE - 4 ||
		_iaSize > INDEX_BUFFER_SIZE  - 6 )
	{
		_flush();
	}

	MyVertex *result = _VertexArray + _vaSize;

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

MyVertex* RenderDirect3D::DrawFan(size_t nEdges)
{
	_ASSERT(nEdges*3 < INDEX_BUFFER_SIZE);

	if( _vaSize + nEdges   > VERTEX_BUFFER_SIZE - 1 ||
		_iaSize + nEdges*3 > INDEX_BUFFER_SIZE )
	{
		_flush();
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
