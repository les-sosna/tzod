#pragma once
#include <math/MyMath.h>
#include <cstdint>

struct SpriteColor
{
	union {
		unsigned char rgba[4];
		uint32_t color;
		struct {
			unsigned char r, g, b, a;
		};
	};

	SpriteColor() = default;
	SpriteColor(uint32_t c) : color(c) {}
	SpriteColor(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_)
		: r(r_)
		, g(g_)
		, b(b_)
		, a(a_)
	{}
};

struct MyLine
{
	vec2d       begin;
	vec2d       end;
	SpriteColor color;
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

enum DisplayOrientation
{
	DO_0,
	DO_90,
	DO_180,
	DO_270,
};

struct MyVertex
{                        // offset  size
	float        x,y,z;  //   0      12
	SpriteColor  color;  //  12       4
	float        u,v;    //  16       8
};

///////////////////////////////////////////////////////////////////////////////

class Image
{
public:
	virtual ~Image() = default;
	virtual const void* GetData() const = 0;
	virtual unsigned int GetBpp() const = 0;
	virtual unsigned int GetWidth() const = 0;
	virtual unsigned int GetHeight() const = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct IRender
{
	virtual ~IRender() = default;
	virtual void OnResizeWnd(unsigned int width, unsigned int height) = 0;
	virtual void SetDisplayOrientation(DisplayOrientation displayOrientation) = 0;

	virtual void SetScissor(const RectRB *rect) = 0;
	virtual void SetViewport(const RectRB *rect) = 0;
	virtual void Camera(const RectRB *vp, float x, float y, float scale) = 0;

	virtual void SetAmbient(float ambient) = 0;
	virtual void SetMode (const RenderMode mode) = 0;
	virtual void Begin   (void) = 0;
	virtual void End     (void) = 0;


	//
	// texture management
	//

	virtual bool TexCreate(DEV_TEXTURE &tex, const Image &img, bool magFilter) = 0;
	virtual void TexFree(DEV_TEXTURE tex) = 0;


	//
	// high level primitive drawing
	//

	virtual MyVertex* DrawQuad(DEV_TEXTURE tex) = 0;
	virtual MyVertex* DrawFan(unsigned int nEdges) = 0;

	virtual void DrawLines(const MyLine *lines, size_t count) = 0;
};
