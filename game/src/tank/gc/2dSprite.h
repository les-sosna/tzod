// 2dSprite.h
/////////////////////////////////////////

#pragma once

#include "Actor.h"
#include "video/TextureManager.h" // TODO: try to remove
#include "globals.h"
#include "constants.h"
#include "render/ObjectView.h"

/////////////////////////////////////////////////

class TextureCache
{
	friend class GC_2dSprite;
	float  width;
	float  height;
	size_t texture;

public:
	TextureCache(const char *name);
	size_t GetTexture() const { return texture; }
};

///////////////////////////////////////////
// flags
#define GC_FLAG_2DSPRITE_VISIBLE               (GC_FLAG_ACTOR_ << 0)
#define GC_FLAG_2DSPRITE_INGRIDSET             (GC_FLAG_ACTOR_ << 1)
#define GC_FLAG_2DSPRITE_DROPSHADOW            (GC_FLAG_ACTOR_ << 2)
#define GC_FLAG_2DSPRITE_                      (GC_FLAG_ACTOR_ << 3)

class GC_2dSprite : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_2dSprite);
    typedef GC_Actor base;

private:
	vec2d _direction;
	SpriteColor  _color;
	size_t _texId;
	unsigned int _frame;

public:
	size_t GetTexture() const { return _texId; }
	SpriteColor GetColor() const { return _color; }
	int   GetFrameCount()   const { return g_texman->Get(_texId).uvFrames.size(); }
	int   GetCurrentFrame() const { return _frame; }
	void  GetGlobalRect(FRECT &rect) const
	{
		const LogicalTexture &lt = g_texman->Get(_texId);
		rect.left   = GetPos().x - lt.pxFrameWidth * lt.uvPivot.x;
		rect.top    = GetPos().y - lt.pxFrameHeight * lt.uvPivot.y;
		rect.right  = rect.left + lt.pxFrameWidth;
		rect.bottom = rect.top  + lt.pxFrameHeight;
	}
	void GetLocalRect(FRECT &rect) const
	{
		const LogicalTexture &lt = g_texman->Get(_texId);
		rect.left   = -lt.uvPivot.x * lt.pxFrameWidth;
		rect.top    = -lt.uvPivot.y * lt.pxFrameHeight;
		rect.right  = rect.left + lt.pxFrameWidth;
		rect.bottom = rect.top + lt.pxFrameHeight;
	}

	void SetTexture(const char *name);
	void SetTexture(const TextureCache &tc);

	void SetFrame(int nFrame);

	void SetOpacity(float x) { SetOpacity1i( int(x * 255.0f) ); }
	void SetOpacity1i(int x) { _color.r = _color.g = _color.b = _color.a = x & 0xff; }
	void SetColor(unsigned char r, unsigned char g, unsigned char b) { _color.r=r; _color.g=g; _color.b=b; }

	const vec2d& GetDirection() const { return _direction; }
	void SetDirection(const vec2d &d) { assert(fabs(d.sqr()-1)<1e-5); _direction = d; }

	void SetShadow(bool bEnable) { SetFlags(GC_FLAG_2DSPRITE_DROPSHADOW, bEnable); }
	bool GetShadow() const { return CheckFlags(GC_FLAG_2DSPRITE_DROPSHADOW); }

public:
	float GetSpriteWidth() const { return g_texman->Get(_texId).pxFrameWidth; }
	float GetSpriteHeight() const { return g_texman->Get(_texId).pxFrameHeight; }

public:
	void SetGridSet(bool bGridSet) { SetFlags(GC_FLAG_2DSPRITE_INGRIDSET, bGridSet); }
	bool GetGridSet() const { return CheckFlags(GC_FLAG_2DSPRITE_INGRIDSET); }

	void SetVisible(bool bShow) { SetFlags(GC_FLAG_2DSPRITE_VISIBLE, bShow); }
	bool GetVisible() const { return CheckFlags(GC_FLAG_2DSPRITE_VISIBLE); }

public:
    DECLARE_GRID_MEMBER();
    
	GC_2dSprite();
	GC_2dSprite(FromFile);
	virtual ~GC_2dSprite();

	virtual enumZOrder GetZ() const { return Z_NONE; }
	
	virtual void Serialize(World &world, SaveFile &f);
	virtual void Draw(DrawingContext &dc, bool editorMode) const;
};

///////////////////////////////////////////////////////////////////////////////
// end of file
