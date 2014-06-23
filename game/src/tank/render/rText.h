#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_Text : public ObjectView
{
public:
	R_Text(TextureManager &tm);
	
	// ObjectView
	virtual enumZOrder GetZ(const GC_Actor &actor) const { return Z_PARTICLE; }
	virtual void Draw(const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _fontDefault;
	size_t _fontDigitsRed;
	size_t _fontDigitsGreen;
};
