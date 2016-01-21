#pragma once
#include <ui/Window.h>

class World;
class Deathmatch;
class LangCache;

class ScoreTable : public UI::Window
{
public:
	ScoreTable(UI::Window *parent, World &world, Deathmatch &deathmatch, LangCache &lang);

protected:
	void OnParentSize(float width, float height) override;
	void Draw(DrawingContext &dc) const override;

private:
	size_t _font;
	World &_world;
	Deathmatch &_deathmatch;
	LangCache &_lang;
};
