// gui_scoretable.h

#pragma once

#include "Base.h"
#include "Window.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class ScoreTable : public Window
{
public:
	ScoreTable(Window *parent);

protected:
	virtual void OnTimeStep(float dt);
	virtual void OnParentSize(float width, float height);
	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;

private:
	size_t _font;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
