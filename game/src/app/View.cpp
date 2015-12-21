#include "inc/app/View.h"
#include "inc/app/AppWindow.h"
#include "inc/app/tzod.h"
#include <as/AppCfg.h>
#include <fs/FileSystem.h>
#include <shell/DesktopFactory.h>
#include <ui/ConsoleBuffer.h>
#include <ui/Window.h>
#include <video/DrawingContext.h>
#include <video/RenderOpenGL.h>
#include <video/TextureManager.h>
#ifndef NOSOUND
#include <audio/SoundView.h>
#endif

static TextureManager InitTextureManager(FS::FileSystem &fs, UI::ConsoleBuffer &logger, IRender &render)
{
	TextureManager textureManager(render);

	if (textureManager.LoadPackage(FILE_TEXTURES, fs.Open(FILE_TEXTURES)->QueryMap(), fs) <= 0)
		logger.Printf(1, "WARNING: no textures loaded");
	if (textureManager.LoadDirectory(DIR_SKINS, "skin/", fs) <= 0)
		logger.Printf(1, "WARNING: no skins found");

	return textureManager;
}

struct TzodViewImpl
{
	TextureManager textureManager;
	UI::LayoutManager gui;
#ifndef NOSOUND
	SoundView soundView;
#endif

	TzodViewImpl(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app, AppWindow &appWindow)
		: textureManager(InitTextureManager(fs, logger, appWindow.GetRender()))
		, gui(appWindow.GetInput(),
			appWindow.GetClipboard(),
			textureManager,
			DesktopFactory(app.GetAppState(),
				app.GetAppController(),
				fs,
				app.GetConf(),
				app.GetLang(),
				logger,
				[]() {}))
#ifndef NOSOUND
        , soundView(app.GetAppState(), *fs.GetFileSystem(DIR_SOUND))
#endif
	{}
};

TzodView::TzodView(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app, AppWindow &appWindow)
	: _appWindow(appWindow)
	, _impl(new TzodViewImpl(fs, logger, app, appWindow))
{
	int width = appWindow.GetPixelWidth();
	int height = appWindow.GetPixelHeight();
	_impl->gui.GetDesktop()->Resize((float)width, (float)height);

	//	ThemeManager themeManager(appState, *fs, texman);

	_appWindow.SetInputSink(&_impl->gui);
}

TzodView::~TzodView()
{
	_appWindow.SetInputSink(nullptr);
}

void TzodView::Step(float dt)
{
	_impl->gui.TimeStep(dt); // this also sends user controller state to WorldController
}

void TzodView::Render(AppWindow &appWindow)
{
#ifndef NOSOUND
//        vec2d pos(0, 0);
//        if (!_world.GetList(LIST_cameras).empty())
//        {
//            _world.GetList(LIST_cameras).for_each([&pos](ObjectList::id_type, GC_Object *o)
//            {
//                pos += static_cast<GC_Camera*>(o)->GetCameraPos();
//            });
//        }
//        soundView.SetListenerPos(pos);
	_impl->soundView.Step();
#endif

	unsigned int width = appWindow.GetPixelWidth();
	unsigned int height = appWindow.GetPixelHeight();

	DrawingContext dc(_impl->textureManager, width, height);
	appWindow.GetRender().Begin();
	_impl->gui.Render(dc);
	appWindow.GetRender().End();
}

UI::LayoutManager& TzodView::GetGui()
{
	return _impl->gui;
}

