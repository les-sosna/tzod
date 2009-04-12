// gui_widgets.h

#pragma once

#include "ui/Text.h"


namespace UI
{

class FpsCounter : public Text
{
	std::list<float> _dts;
	std::list<float> _dts_net;
	int _nSprites;
	int _nLights;
	int _nBatches;

public:
	FpsCounter(Window *parent, float x, float y, enumAlignText align);

protected:
	void OnVisibleChange(bool visible);
	void OnTimeStep(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class TimeElapsed : public Text
{
public:
	TimeElapsed(Window *parent, float x, float y, enumAlignText align);
protected:
	void OnVisibleChange(bool visible);
	void OnTimeStep(float dt);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
