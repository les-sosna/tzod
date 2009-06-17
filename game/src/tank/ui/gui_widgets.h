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
	void OnVisibleChange(bool visible, bool inherited);
	void OnTimeStep(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class TimeElapsed : public Text
{
public:
	TimeElapsed(Window *parent, float x, float y, enumAlignText align);
protected:
	void OnVisibleChange(bool visible, bool inherited);
	void OnTimeStep(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class Oscilloscope : public Window
{
public:
	Oscilloscope(Window *parent, float x, float y);
	void Push(float value);

protected:
	void DrawChildren(float sx, float sy) const;

private:
	size_t _barTexture;
	std::deque<float> _data;
	float _scale;
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
