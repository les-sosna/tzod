#pragma once
#include "detail/ConfigConsoleHistory.h"
#include "detail/MapCollection.h"
#include <as/AppStateListener.h>
#include <render/RenderScheme.h>
#include <render/WorldView.h>
#include "ui/Navigation.h"
#include <ui/Window.h>
#include <functional>
#include <string>

namespace FS
{
	class FileSystem;
}

namespace Plat
{
	struct AppWindowCommandClose;
}

namespace UI
{
	class Console;
	class Oscilloscope;
	class Button;
	class Text;
}

class AppConfig;
class AppController;
class MainMenuDlg;
class EditorMain;
class GameLayout;
class FpsCounter;
class ShellConfig;
class LangCache;
class LuaConsole;
class NavStack;

class Desktop final
	: public UI::Window
	, private UI::Managerful
	, private UI::KeyboardSink
	, private UI::NavigationSink
	, private AppStateListener
{
public:
	Desktop(UI::TimeStepManager &manager,
	        TextureManager &texman,
	        AppState &appState,
	        AppConfig &appConfig,
	        AppController &appController,
	        FS::FileSystem &fs,
	        ShellConfig &conf,
	        LangCache &lang,
	        DMCampaign &dmCampaign,
	        Plat::ConsoleBuffer &logger,
			Plat::AppWindowCommandClose* cmdClose = nullptr);
	virtual ~Desktop();

	void ShowConsole(bool show);

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;

protected:
	UI::NavigationSink* GetNavigationSink() override { return this; }
	UI::KeyboardSink *GetKeyboardSink() override { return this; }

private:
	ConfigConsoleHistory _history;
	TextureManager &_texman;
	AppConfig &_appConfig;
	AppController &_appController;
	FS::FileSystem &_fs;
	ShellConfig &_conf;
	LangCache &_lang;
	DMCampaign &_dmCampaign;
	Plat::ConsoleBuffer &_logger;
	Plat::AppWindowCommandClose* _cmdCloseAppWindow;
	std::unique_ptr<LuaConsole> _luaConsole;

	std::shared_ptr<EditorMain> _editor;
	std::shared_ptr<GameLayout> _game;
	std::shared_ptr<UI::Text> _tierTitle;
	std::shared_ptr<UI::Rectangle> _background;
	std::shared_ptr<UI::Console> _con;
	std::shared_ptr<FpsCounter> _fps;
	std::shared_ptr<UI::Button> _pauseButton;
	std::shared_ptr<NavStack> _navStack;

	RenderScheme _renderScheme;
	WorldView _worldView;
	MapCollection _mapCollection;

	void OnNewCampaign();
	void OnSinglePlayer();
	void OnSplitScreen();
	void OnOpenMap();
	void OnExportMap();
	void OnSettingsMain();
	void OnPlayerSettings();
	void OnControlsSettings();
	void OnAdvancedSettings();
	void OnMapSettings();
	bool GetEditorMode() const;
	void SetEditorMode(bool editorMode);
	void ShowMainMenu();

	void OnChangeShowFps();

	void OnCommand(std::string_view cmd);
	bool OnCompleteCommand(std::string_view cmd, int &pos, std::string &result);

	void OnCloseChild(std::shared_ptr<UI::Window> child);

	void UpdateFocus();

	void NavigateHome();
	void NavigateBack();
	bool CanNavigateBack() const;

	// AppStateListener
	void OnGameContextChanging() override;
	void OnGameContextChanged() override;

	// UI::KeyboardSink
	bool OnKeyPressed(const UI::InputContext &ic, Plat::Key key) override;
	void OnKeyReleased(const UI::InputContext &ic, Plat::Key key) override;

	// UI::NavigationSink
	bool CanNavigate(TextureManager& texman, const UI::InputContext& ic, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate) const override;
	void OnNavigate(TextureManager& texman, const UI::InputContext& ic, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate, UI::NavigationPhase phase) override;
};
