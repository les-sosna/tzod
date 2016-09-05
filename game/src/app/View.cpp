#include "inc/app/View.h"
#include "inc/app/tzod.h"
#include <as/AppConstants.h>
#include <fs/FileSystem.h>
#include <shell/Desktop.h>
#include <ui/AppWindow.h>
#include <ui/ConsoleBuffer.h>
#include <ui/GuiManager.h>
#include <ui/InputContext.h>
#include <ui/LayoutContext.h>
#include <ui/StateContext.h>
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

	if (textureManager.LoadPackage(ParsePackage(FILE_TEXTURES, fs.Open(FILE_TEXTURES)->QueryMap(), fs)) <= 0)
		logger.Printf(1, "WARNING: no textures loaded");
	if (textureManager.LoadPackage(ParseDirectory(DIR_SKINS, "skin/", fs)) <= 0)
		logger.Printf(1, "WARNING: no skins found");

	return textureManager;
}

struct TzodViewImpl
{
	TextureManager textureManager;
	UI::InputContext inputContext;
	UI::LayoutManager gui;
	std::shared_ptr<UI::Window> desktop;
#ifndef NOSOUND
	SoundView soundView;
#endif

	TzodViewImpl(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app, AppWindow &appWindow)
		: textureManager(InitTextureManager(fs, logger, appWindow.GetRender()))
		, inputContext(appWindow.GetInput(), appWindow.GetClipboard())
		, gui(textureManager, inputContext)
		, desktop(std::make_shared<Desktop>(
			gui,
			textureManager,
			app.GetAppState(),
			app.GetAppConfig(),
			app.GetAppController(),
			fs,
			app.GetConf(),
			app.GetLang(),
			app.GetDMCampaign(),
			logger))
#ifndef NOSOUND
		, soundView(*fs.GetFileSystem(DIR_SOUND), logger, app.GetAppState())
#endif
	{
		gui.SetDesktop(desktop);
	}
};

static AppWindow& EnsureCurrent(AppWindow &appWindow)
{
	appWindow.MakeCurrent();
	return appWindow;
}

TzodView::TzodView(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app, AppWindow &appWindow)
	: _appWindow(EnsureCurrent(appWindow))
	, _impl(new TzodViewImpl(fs, logger, app, appWindow))
{
	int width = appWindow.GetPixelWidth();
	int height = appWindow.GetPixelHeight();
	float layoutScale = appWindow.GetLayoutScale();
	_impl->desktop->Resize((float)width / layoutScale, (float)height / layoutScale);

	//	ThemeManager themeManager(appState, *fs, texman);

	_appWindow.SetInputSink(&_impl->gui);
}

TzodView::~TzodView()
{
	_appWindow.MakeCurrent();
	_appWindow.SetInputSink(nullptr);
}

void TzodView::Step(float dt)
{
	_appWindow.MakeCurrent();
	_impl->gui.TimeStep(dt); // this also sends user controller state to WorldController
}

void TzodView::Render(AppWindow &appWindow)
{
	_appWindow.MakeCurrent();

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
	float layoutScale = appWindow.GetLayoutScale();

	DrawingContext dc(_impl->textureManager, width, height);
	appWindow.GetRender().Begin();

	UI::StateContext stateContext;
	UI::LayoutContext layoutContext(layoutScale, vec2d{}, vec2d{ static_cast<float>(width), static_cast<float>(height) }, _impl->gui.GetDesktop()->GetEnabled());
	UI::RenderSettings rs{ stateContext, _impl->gui.GetInputContext(), dc, _impl->textureManager };

	UI::RenderUIRoot(*_impl->gui.GetDesktop(), rs, layoutContext);

#ifndef NDEBUG
	for (auto &id2pos : rs.ic.GetLastPointerLocation())
	{
		FRECT dst = { id2pos.second.x - 4, id2pos.second.y - 4, id2pos.second.x + 4, id2pos.second.y + 4 };
		rs.dc.DrawSprite(dst, 0U, 0xffffffff, 0U);
	}
#endif

	appWindow.GetRender().End();
}

