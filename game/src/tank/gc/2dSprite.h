// 2dSprite.h

#pragma once

#include "Actor.h"
#include "render/ObjectView.h"

#define GC_FLAG_2DSPRITE_                      (GC_FLAG_ACTOR_ << 0)

class GC_2dSprite : public GC_Actor
{
    typedef GC_Actor base;

	size_t _texId;

public:
	void GetGlobalRect(FRECT &rect) const;

	void SetTexture(const char *name);

	float GetSpriteWidth() const;
	float GetSpriteHeight() const;

public:
    DECLARE_GRID_MEMBER();
    
	GC_2dSprite();
	virtual ~GC_2dSprite() = 0;

	virtual enumZOrder GetZ() const { return Z_NONE; }
	
	virtual void Serialize(World &world, SaveFile &f);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
