#include "TzodViewImpl.h"
#include "inc/app/tzod.h"
#include <as/AppConstants.h>
#include <fs/FileSystem.h>
#include <shell/Config.h>
#include <shell/Desktop.h>
#include <shell/Profiler.h>
#include <plat/ConsoleBuffer.h>
#ifndef NOSOUND
# include <audio/SoundView.h>
#endif
#include <numeric>

static TextureManager InitTextureManager(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, IRender &render)
{
	TextureManager textureManager(render);

	if (textureManager.LoadPackage(ParsePackage(FILE_TEXTURES, fs.Open(FILE_TEXTURES)->QueryMap(), fs)) <= 0)
		logger.Printf(1, "WARNING: no textures loaded");
	if (textureManager.LoadPackage(ParseDirectory(DIR_SKINS, "skin/", fs)) <= 0)
		logger.Printf(1, "WARNING: no skins found");

	return textureManager;
}

TzodViewImpl::TzodViewImpl(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, TzodApp &app, Plat::AppWindow &appWindow)
	: _app(app)
	, _textureManager(InitTextureManager(fs, logger, appWindow.GetRender()))
	, _timeStepManager()
	, _desktop(std::make_shared<Desktop>(
		_timeStepManager,
		_textureManager,
		app.GetAppState(),
		app.GetAppConfig(),
		app.GetAppController(),
		fs,
		app.GetShellConfig(),
		app.GetLang(),
		app.GetDMCampaign(),
		logger))
	, _uiInputRenderingController(appWindow, _textureManager, _timeStepManager, _desktop)
#ifndef NOSOUND
	, _soundView(app.GetShellConfig().s_enabled.Get() ? std::make_unique<SoundView>(*fs.GetFileSystem(DIR_SOUND), logger, app.GetAppState()) : nullptr)
#endif
{
}

TzodViewImpl::~TzodViewImpl()
{
}


static CounterBase counterDt("dt", "dt, ms");

void TzodViewImpl::Step(float dt)
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

	// controller pass
	_uiInputRenderingController.TimeStep(filteredDt); // this also sends user controller state to WorldController

	// model pass
	_app.Step(filteredDt);

	// view pass
#ifndef NOSOUND
	if (_soundView)
	{
//		vec2d pos(0, 0);
//		if (!_world.GetList(LIST_cameras).empty())
//		{
//			_world.GetList(LIST_cameras).for_each([&pos](ObjectList::id_type, GC_Object *o)
//			{
//				pos += static_cast<GC_Camera*>(o)->GetCameraPos();
//			});
//		}
//		_soundView->SetListenerPos(pos);
		_soundView->Step();
	}
#endif
	_uiInputRenderingController.OnRefresh();
}
