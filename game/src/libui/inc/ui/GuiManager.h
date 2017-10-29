#pragma once
#include "PointerInput.h"
#include <math/MyMath.h>
#include <list>
#include <memory>
#include <vector>

class RenderContext;
class TextureManager;

namespace UI
{

class DataContext;
class InputContext;
class LayoutContext;
class TimeStepping;
class StateContext;
class Window;

struct RenderSettings
{
	InputContext &ic;
	RenderContext &rc;
	TextureManager &texman;
	float time;
	std::vector<std::shared_ptr<Window>> hoverPath;
	bool topMostPass;
};

void RenderUIRoot(Window &desktop, RenderSettings &rs, const LayoutContext &lc, const DataContext &dc, const StateContext &sc);

class LayoutManager
{
public:
	LayoutManager(TextureManager &texman, InputContext &ic);
	~LayoutManager();

	void TimeStep(float dt);

	InputContext& GetInputContext() const { return _inputContext; }
	TextureManager& GetTextureManager() const { return _texman; }
	float GetTime() const { return _time; }

	std::shared_ptr<Window> GetDesktop() const { return _desktop; }
	void SetDesktop(std::shared_ptr<Window> desktop) { _desktop = std::move(desktop); }

private:
	friend class TimeStepping;
	std::list<TimeStepping*>::iterator TimeStepRegister(TimeStepping* wnd);
	void TimeStepUnregister(std::list<TimeStepping*>::iterator it);

	TextureManager &_texman;
	InputContext &_inputContext;

	std::list<TimeStepping*> _timestep;
	std::list<TimeStepping*>::iterator _tsCurrent;
	bool _tsDeleteCurrent;
	float _time = 0;

	std::shared_ptr<Window> _desktop;
};

} // namespace UI
