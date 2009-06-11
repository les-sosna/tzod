// gui_scoretable.h

#pragma once

#include "Base.h"
#include "Window.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class ScoreTable : public Window
{
	size_t _font;

public:
	ScoreTable(Window *parent);
	virtual ~ScoreTable();

protected:
	virtual void OnTimeStep(float dt);
	virtual void OnParentSize(float width, float height);
	virtual void DrawChildren(float sx, float sy) const;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
