#include "rText.h"
#include <gc/GameClasses.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

R_Text::R_Text(TextureManager &tm)
	: _fontDefault(tm.FindSprite("font_default"))
	, _fontDigitsRed(tm.FindSprite("font_digits_red"))
	, _fontDigitsGreen(tm.FindSprite("font_digits_green"))
{
}

void R_Text::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_Text*>(&mo));
	auto &text = static_cast<const GC_Text&>(mo);
	vec2d pos = text.GetPos();
	size_t font;
	switch (text.GetStyle())
	{
		case GC_Text::SCORE_PLUS: font = _fontDigitsGreen; break;
		case GC_Text::SCORE_MINUS: font = _fontDigitsRed; break;
		default: font = _fontDefault;
	}
	rc.DrawBitmapText(pos, 1.f, font, 0xffffffff, text.GetText(), alignTextCC);
}
