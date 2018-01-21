#include "TzodViewImpl.h"
#include "inc/app/tzod.h"
#include <as/AppConstants.h>
#include <fs/FileSystem.h>
#include <shell/Desktop.h>
#include <shell/Profiler.h>
#include <ui/ConsoleBuffer.h>

static TextureManager InitTextureManager(FS::FileSystem &fs, UI::ConsoleBuffer &logger, IRender &render)
{
	TextureManager textureManager(render);

	if (textureManager.LoadPackage(ParsePackage(FILE_TEXTURES, fs.Open(FILE_TEXTURES)->QueryMap(), fs)) <= 0)
		logger.Printf(1, "WARNING: no textures loaded");
	if (textureManager.LoadPackage(ParseDirectory(DIR_SKINS, "skin/", fs)) <= 0)
		logger.Printf(1, "WARNING: no skins found");

	return textureManager;
}

TzodViewImpl::TzodViewImpl(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app, AppWindow &appWindow)
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
	, _soundView(*fs.GetFileSystem(DIR_SOUND), logger, app.GetAppState())
#endif
{
}


static CounterBase counterDt("dt", "dt, ms");

void TzodViewImpl::Step(float dt)
{
	// controller pass
	counterDt.Push(dt);
	_uiInputRenderingController.TimeStep(dt); // this also sends user controller state to WorldController

	// model pass
	_app.Step(dt);

	// view pass
#ifndef NOSOUND
//	vec2d pos(0, 0);
//	if (!_world.GetList(LIST_cameras).empty())
//	{
//		_world.GetList(LIST_cameras).for_each([&pos](ObjectList::id_type, GC_Object *o)
//		{
//			pos += static_cast<GC_Camera*>(o)->GetCameraPos();
//		});
//	}
//	_soundView.SetListenerPos(pos);
	_soundView.Step();
#endif
	_uiInputRenderingController.OnRefresh();
}
