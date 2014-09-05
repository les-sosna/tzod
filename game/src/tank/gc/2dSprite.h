// 2dSprite.h

#pragma once

#include "Actor.h"
#include "render/ObjectView.h"

#define GC_FLAG_2DSPRITE_                      (GC_FLAG_ACTOR_ << 0)

class GC_2dSprite : public GC_Actor
{
    typedef GC_Actor base;
public:
    DECLARE_GRID_MEMBER();
	virtual enumZOrder GetZ() const { return Z_NONE; }
};

// end of file
