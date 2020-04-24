#include "TzodViewImpl.h"
#include "inc/app/View.h"
#include "inc/app/tzod.h"
#include <shell/Desktop.h>
#include <shell/Profiler.h>
#ifndef NOSOUND
# include <audio/SoundView.h>
#endif
#include <numeric>

static CounterBase counterDt("raw dt", "raw dt, ms");
static CounterBase counterDtFiltered("dt", "dt, ms");

TzodView::TzodView(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, TzodApp &app, Plat::AppWindowCommandClose *cmdClose)
	: _impl(new TzodViewImpl(fs, cmdClose, logger, app))
{
	//	ThemeManager themeManager(appState, *fs, texman);
}

TzodView::~TzodView()
{
}

void TzodView::Step(TzodApp &app, float dt, const Plat::Input &input)
{
	counterDt.Push(dt);

	// moving average
	_movingAverageWindow.push_back(dt);
	if (_movingAverageWindow.size() > 8)
		_movingAverageWindow.pop_front();
	float mean = std::accumulate(_movingAverageWindow.begin(), _movingAverageWindow.end(), 0.0f) / (float)_movingAverageWindow.size();

	// moving median of moving average
	_movingMedianWindow.push_back(mean);
	if (_movingMedianWindow.size() > 100)
		_movingMedianWindow.pop_front();
	float buf[100];
	std::copy(_movingMedianWindow.begin(), _movingMedianWindow.end(), buf);
	std::nth_element(buf, buf + _movingMedianWindow.size() / 2, buf + _movingMedianWindow.size());

	// median
	float filteredDt = buf[_movingMedianWindow.size() / 2];
    counterDtFiltered.Push(filteredDt);

	// controller pass - handle input
	// this also sends user controller state to WorldController
	// note: the game view camera updates as part of the UI which is not ideal.
	//       Consider updating cameras after the world step instead.
	_impl->uiInputRenderingController.TimeStep(filteredDt, input);

	// simulate world
	app.Step(filteredDt);

	// handle world events
#ifndef NOSOUND
	if (_impl->soundView)
	{
		auto listenerPos = _impl->desktop->GetListenerPos();
		_impl->soundView->Step(listenerPos);
	}
#endif
}

Plat::AppWindowInputSink& TzodView::GetAppWindowInputSink()
{
	return _impl->uiInputRenderingController;
}
