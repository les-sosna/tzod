#pragma once
#include "ui/Text.h"
#include <list>
#include <string>
#include <queue>

class AppState;
class TextureManager;

class FpsCounter : public UI::Text
{
	std::list<float> _dts;
	std::list<float> _dts_net;
	int _nSprites;
	int _nLights;
	int _nBatches;
	AppState &_appState;

public:
	FpsCounter(UI::LayoutManager &manager, TextureManager &texman, float x, float y, enumAlignText align, AppState &appState);

protected:
	void OnTimeStep(UI::LayoutManager &manager, float dt);
};

class Oscilloscope : public UI::Window
{
public:
	Oscilloscope(UI::LayoutManager &manager, TextureManager &texman, float x, float y);
	void Push(float value);
	void SetRange(float rmin, float rmax);
	void SetTitle(const std::string &title);
	void SetGridStep(float stepX, float stepY);

	void AutoGrid();
	void AutoRange();

protected:
	void Draw(bool focused, bool enabled, vec2d size, DrawingContext &dc, TextureManager &texman) const override;

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

