#pragma once
#include "inc/render/ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_Text : public ObjectRFunc
{
public:
	R_Text(TextureManager &tm);
	void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

private:
	size_t _fontDefault;
	size_t _fontDigitsRed;
	size_t _fontDigitsGreen;
};
