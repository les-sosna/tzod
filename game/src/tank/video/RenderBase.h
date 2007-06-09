// RenderBase.h

#pragma once

///////////////////////////////////////////////////////////////////////////////

struct DisplayMode
{
    UINT Width;
    UINT Height;
    UINT RefreshRate;
    UINT BitsPerPixel;
};

struct DEV_TEXTURE
{
	union
	{
		unsigned int index;
		void          *ptr;
	};
	bool operator < (const DEV_TEXTURE &r) const
	{
		return ptr < r.ptr;
	}
	bool operator == (const DEV_TEXTURE &r) const
	{
		return ptr == r.ptr;
	}
};

enum RenderMode
{
	RM_LIGHT         = 0,
	RM_WORLD         = 1,
	RM_INTERFACE     = 2,
	//-------------------
	RM_FORCE32BIT    = 0xffffffff
};

struct MyVertex
{                        // offset  size
	float        x,y,z;  //   0      12
	SpriteColor  color;  //  12       4
	float        u,v;    //  16       8
};


///////////////////////////////////////////////////////////////////////////////

struct IRender
{
	// return TRUE if ok
	virtual BOOL Init(HWND hWnd, const DisplayMode *pMode, BOOL bFullScreen) = 0;
	virtual void Release() = 0;

	virtual BOOL  getDisplayMode(DWORD index, DisplayMode *pMode) const = 0;
	virtual DWORD getModeCount() const  = 0;

	virtual void OnResizeWnd() = 0;

	virtual void SetViewport(const RECT *rect) = 0;
	virtual void Camera(float x, float y, float scale, float angle) = 0;

    virtual int  GetWidth() const = 0;
    virtual int  GetHeight() const = 0;

    virtual int  GetViewportWidth() const = 0;
    virtual int  GetViewportHeight() const = 0;

	virtual void SetMode (const RenderMode mode) = 0;
	virtual void Begin   (void) = 0;
	virtual void End     (void) = 0;

	virtual void SetAmbient(float ambient) = 0;

	virtual bool TakeScreenshot(TCHAR *fileName) = 0;


	//
	// texture management
	//

	virtual bool TexCreate(DEV_TEXTURE &tex, const TextureData *pData) = 0;
	virtual void TexFree(DEV_TEXTURE tex) = 0;
	virtual void TexBind(DEV_TEXTURE tex) = 0;


	//
	// high level primitive drawing
	//

	virtual MyVertex* DrawQuad() = 0;
	virtual MyVertex* DrawFan(size_t nEdges) = 0;

};


// end of file
