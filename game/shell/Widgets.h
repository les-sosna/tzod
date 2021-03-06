#pragma once
#include "ui/Rectangle.h"
#include "ui/Text.h"
#include <list>
#include <string>
#include <queue>
#include <cfloat>

class AppState;
class TextureManager;

class FpsCounter final
	: public UI::Text
	, private UI::TimeStepping
{
public:
	FpsCounter(UI::TimeStepManager &manager, enumAlignText align, AppState &appState);

protected:
	void OnTimeStep(const Plat::Input &input, bool focused, float dt);

private:
	float _minDt = FLT_MAX;
	float _maxDt = 0;
	float _totalTime = 0;
	unsigned int _totalSteps = 0;

	int _nSprites;
	int _nLights;
	int _nBatches;
	AppState &_appState;
};

class Oscilloscope final
	: public UI::Rectangle
{
public:
	Oscilloscope();
	void Push(TextureManager &texman, float value);
	void SetRange(float rmin, float rmax);
	void SetTitle(std::string title);
	void SetGridStep(float stepX, float stepY);

	void AutoGrid(TextureManager &texman);
	void AutoRange();

protected:
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const override;

private:
	UI::Texture _titleFont = "font_small";
	std::string _title;
	std::deque<float> _data;
	float _rangeMin;
	float _rangeMax;
	float _gridStepX;
	float _gridStepY;
	float _stepSize;
};

