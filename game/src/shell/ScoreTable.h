#pragma once
#include <ui/Texture.h>
#include <ui/Window.h>

class World;
class Deathmatch;
class LangCache;
class TextureManager;

class ScoreTable final
	: public UI::Window
{
public:
	ScoreTable(World &world, const Deathmatch *deathmatch, LangCache &lang);

protected:
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;

private:
	UI::Texture _font = "font_default";
	UI::Texture _texHighlight = "ui/selection";
	World &_world;
	const Deathmatch *_deathmatch;
	LangCache &_lang;
};
