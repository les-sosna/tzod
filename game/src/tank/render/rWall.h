#pragma once
#include "ObjectView.h"
#include "constants.h" // FIXME: enumZOrder

#include <stddef.h>

class TextureManager;

class R_Wall : public ObjectView
{
public:
	R_Wall(TextureManager &tm, const char *tex);
	
	// ObjectView
	virtual void Draw(const GC_Actor &actor, DrawingContext &dc, bool editorMode) const override;
	
private:
	enum {WALL, LT, RT, RB, LB};
	size_t _texId[5];
	TextureManager &_tm;
};

