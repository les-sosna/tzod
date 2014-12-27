#pragma once

#include <ui/Window.h>

class World;
class Deathmatch;

namespace UI
{

class ScoreTable : public Window
{
public:
	ScoreTable(Window *parent, World &world, Deathmatch &deathmatch);

protected:
	virtual void OnParentSize(float width, float height);
	virtual void DrawChildren(DrawingContext &dc, float sx, float sy) const;

private:
	size_t _font;
    World &_world;
	Deathmatch &_deathmatch;
};

} // end of namespace UI
