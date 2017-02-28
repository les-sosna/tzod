#pragma once
#include "detail/ConfigConsoleHistory.h"
#include <as/AppStateListener.h>
#include <luaetc/LuaDeleter.h>
#include <render/RenderScheme.h>
#include <render/WorldView.h>
#include <ui/Window.h>
#include <functional>
#include <string>

namespace FS
{
	class FileSystem;
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
class EditorLayout;
class GameLayout;
class FpsCounter;
class ShellConfig;
class LangCache;
class NavStack;

class Desktop
	: public UI::Window
	, private UI::Managerful
	, private UI::KeyboardSink
	, private AppStateListener
{
public:
	Desktop(UI::LayoutManager &manager,
	        TextureManager &texman,
	        AppState &appState,
	        AppConfig &appConfig,
	        AppController &appController,
	        FS::FileSystem &fs,
	        ShellConfig &conf,
	        LangCache &lang,
	        DMCampaign &dmCampaign,
	        UI::ConsoleBuffer &logger);
	virtual ~Desktop();

	void ShowConsole(bool show);

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	float GetChildOpacity(const UI::Window &child) const override;

protected:
	UI::KeyboardSink *GetKeyboardSink() override { return this; }
	void OnTimeStep(UI::LayoutManager &manager, float dt) override;

private:
	ConfigConsoleHistory  _history;
	TextureManager &_texman;
	AppConfig &_appConfig;
	AppController &_appController;
	FS::FileSystem &_fs;
	ShellConfig &_conf;
	LangCache &_lang;
	DMCampaign &_dmCampaign;
	UI::ConsoleBuffer &_logger;
	std::unique_ptr<lua_State, LuaStateDeleter> _globL;

	std::shared_ptr<EditorLayout> _editor;
	std::shared_ptr<GameLayout> _game;
	std::shared_ptr<UI::Text> _tierTitle;
	std::shared_ptr<UI::Rectangle> _background;
	std::shared_ptr<UI::Console> _con;
	std::shared_ptr<FpsCounter> _fps;
	std::shared_ptr<UI::Button> _pauseButton;
	std::shared_ptr<NavStack> _navStack;

	RenderScheme _renderScheme;
	WorldView _worldView;

	void OnNewCampaign();
	void OnNewDM();
	void OnOpenMap();
	void OnExportMap();
	void OnGameSettings();
	void OnMapSettings();
	bool GetEditorMode() const;
	void SetEditorMode(bool editorMode);
	void ShowMainMenu();

	void OnChangeShowFps();

	void OnCommand(const std::string &cmd);
	bool OnCompleteCommand(const std::string &cmd, int &pos, std::string &result);

	void OnCloseChild(std::shared_ptr<UI::Window> child);

	void UpdateFocus();

	// AppStateListener
	void OnGameContextChanging() override;
	void OnGameContextChanged() override;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;
};
