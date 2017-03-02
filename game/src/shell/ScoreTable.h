#pragma once
#include <ui/Rectangle.h>

class World;
class Deathmatch;
class LangCache;
class TextureManager;

class ScoreTable : public UI::Rectangle
{
public:
	ScoreTable(TextureManager &texman, World &world, const Deathmatch *deathmatch, LangCache &lang);

protected:
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;

private:
	size_t _font;
	size_t _texHighlight;
	World &_world;
	const Deathmatch *_deathmatch;
	LangCache &_lang;
};
