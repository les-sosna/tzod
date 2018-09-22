#include "TzodViewImpl.h"
#include "inc/app/View.h"

static Plat::AppWindow& EnsureCurrent(Plat::AppWindow &appWindow)
{
	appWindow.MakeCurrent();
	return appWindow;
}

TzodView::TzodView(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app, Plat::AppWindow &appWindow)
	: _appWindow(EnsureCurrent(appWindow))
	, _impl(new TzodViewImpl(fs, logger, app, appWindow))
{
	//	ThemeManager themeManager(appState, *fs, texman);
}

TzodView::~TzodView()
{
	_appWindow.MakeCurrent();
}

void TzodView::Step(float dt)
{
	_impl->Step(dt);
}
