// 2dSprite.h

#pragma once

#include "Actor.h"
#include "video/TextureManager.h" // TODO: try to remove
#include "globals.h"
#include "render/ObjectView.h"

#define GC_FLAG_2DSPRITE_INGRIDSET             (GC_FLAG_ACTOR_ << 0)
#define GC_FLAG_2DSPRITE_                      (GC_FLAG_ACTOR_ << 1)

class GC_2dSprite : public GC_Actor
{
    typedef GC_Actor base;

	vec2d _direction;
	size_t _texId;

public:
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

	const vec2d& GetDirection() const { return _direction; }
	void SetDirection(const vec2d &d) { assert(fabs(d.sqr()-1)<1e-5); _direction = d; }

	float GetSpriteWidth() const { return g_texman->Get(_texId).pxFrameWidth; }
	float GetSpriteHeight() const { return g_texman->Get(_texId).pxFrameHeight; }

	void SetGridSet(bool bGridSet) { SetFlags(GC_FLAG_2DSPRITE_INGRIDSET, bGridSet); }
	bool GetGridSet() const { return CheckFlags(GC_FLAG_2DSPRITE_INGRIDSET); }

public:
    DECLARE_GRID_MEMBER();
    
	GC_2dSprite();
	virtual ~GC_2dSprite() = 0;

	virtual enumZOrder GetZ() const { return Z_NONE; }
	
	virtual void Serialize(World &world, SaveFile &f);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
