#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_Vehicle : public ObjectView
{
public:
	R_Vehicle(TextureManager &tm);
	
	// ObjectView
	virtual enumZOrder GetZ(const GC_Actor &actor) const { return Z_VEHICLES; }
	virtual void Draw(const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _nameFont;
};

