// gui_scoretable.h

#pragma once

#include "Window.h"

class Level;

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class ScoreTable : public Window
{
public:
	ScoreTable(Window *parent, Level &world);

protected:
	virtual void OnTimeStep(float dt);
	virtual void OnParentSize(float width, float height);
	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;

private:
	size_t _font;
    Level &_world;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
