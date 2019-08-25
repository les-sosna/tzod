#pragma once
#include "PointerInput.h"
#include <math/MyMath.h>
#include <list>
#include <memory>
#include <vector>

class RenderContext;
class TextureManager;

namespace Plat
{
	struct Input;
}

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
	const InputContext &ic;
	RenderContext &rc;
	TextureManager &texman;
	float time;
	std::vector<std::shared_ptr<Window>> hoverPath;
	bool topMostPass;
};

void RenderUIRoot(const std::shared_ptr<Window> &desktop, RenderSettings &rs, const LayoutContext &lc, const DataContext &dc, const StateContext &sc);

class TimeStepManager
{
public:
	TimeStepManager();

	void TimeStep(std::shared_ptr<Window> desktop, float dt, const Plat::Input& input);
	float GetTime() const { return _time; }

private:
	friend class TimeStepping;
	std::list<TimeStepping*>::iterator TimeStepRegister(TimeStepping* wnd);
	void TimeStepUnregister(std::list<TimeStepping*>::iterator it);

	std::list<TimeStepping*> _timestep;
	std::list<TimeStepping*>::iterator _tsCurrent;
	bool _tsDeleteCurrent;
	float _time = 0;
};

} // namespace UI
