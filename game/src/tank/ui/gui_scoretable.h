// gui_scoretable.h

#pragma once

#include "Base.h"
#include "Window.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class ScoreTable : public Window
{
	// PlayerDesc::index будет использоваться для хранения фрагов
	std::vector<PlayerDesc> _players;
	void Refresh();

	Text *_text;

public:
	ScoreTable(Window *parent);
	~ScoreTable();

protected:
	void OnTimeStep(float dt);
	void OnParentSize(float width, float height);
	void DrawChildren(float sx, float sy);
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
