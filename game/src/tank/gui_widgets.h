// gui_widgets.h

#pragma once

#include "ui/Text.h"
#include <list>
#include <string>
#include <queue>

class Level;

namespace UI
{

class FpsCounter : public Text
{
	std::list<float> _dts;
	std::list<float> _dts_net;
	int _nSprites;
	int _nLights;
	int _nBatches;
    Level &_world;

public:
	FpsCounter(Window *parent, float x, float y, enumAlignText align, Level &world);

protected:
	void OnVisibleChange(bool visible, bool inherited);
	void OnTimeStep(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class TimeElapsed : public Text
{
    Level &_world;
public:
	TimeElapsed(Window *parent, float x, float y, enumAlignText align, Level &world);
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
	void SetRange(float rmin, float rmax);
	void SetTitle(const std::string &title);
	void SetGridStep(float stepX, float stepY);

	void AutoGrid();
	void AutoRange();

protected:
	void DrawChildren(const DrawingContext *dc, float sx, float sy) const;

private:
	size_t _barTexture;
	size_t _titleFont;
	std::string _title;
	std::deque<float> _data;
	float _rangeMin;
	float _rangeMax;
	float _gridStepX;
	float _gridStepY;
	float _scale;
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
