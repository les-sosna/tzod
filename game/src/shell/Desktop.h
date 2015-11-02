#pragma once
#include "ConfigConsoleHistory.h"
#include "DefaultCamera.h"
#include <app/AppStateListener.h>
#include <render/RenderScheme.h>
#include <render/WorldView.h>
#include <ui/Window.h>
#include <functional>
#include <string>

namespace FS
{
	class FileSystem;
}
struct lua_State;

struct LuaStateDeleter
{
	void operator()(lua_State *L);
};

namespace UI
{
	class Console;
	class Oscilloscope;
}

class AppController;
class MainMenuDlg;
class EditorLayout;
class GameLayout;
class FpsCounter;
class ConfCache;
class LangCache;

class Desktop
	: public UI::Window
	, private AppStateListener
{
public:
	Desktop(UI::LayoutManager* manager,
	        AppState &appState,
	        AppController &appController,
	        FS::FileSystem &fs,
	        ConfCache &conf,
	        LangCache &lang,
	        UI::ConsoleBuffer &logger,
	        std::function<void()> exitCommand);
	virtual ~Desktop();

	void ShowConsole(bool show);

	void OnCloseChild(int result);

protected:
	bool OnKeyPressed(UI::Key key) override;
	bool OnFocus(bool focus) override;
	void OnSize(float width, float height) override;
	void OnTimeStep(float dt) override;

private:
	ConfigConsoleHistory  _history;
	AppController &_appController;
	FS::FileSystem &_fs;
	ConfCache &_conf;
	LangCache &_lang;
	UI::ConsoleBuffer &_logger;
	std::function<void()> _exitCommand;
	std::unique_ptr<lua_State, LuaStateDeleter> _globL;

	MainMenuDlg  *_mainMenu = nullptr;
	EditorLayout *_editor = nullptr;
	GameLayout   *_game = nullptr;
	UI::Console  *_con = nullptr;
	FpsCounter   *_fps = nullptr;

	int _nModalPopups;

	RenderScheme _renderScheme;
	WorldView _worldView;
	DefaultCamera _defaultCamera;

	void OnNewCampaign();
	void OnNewDM();
	void OnNewMap();
	void OnOpenMap(std::string fileName);
	void OnExportMap(std::string fileName);
	bool GetEditorMode() const;
	void SetEditorMode(bool editorMode);
	bool IsGamePaused() const;
	void ShowMainMenu(bool show);

	void OnChangeShowFps();

	void OnCommand(const std::string &cmd);
	bool OnCompleteCommand(const std::string &cmd, int &pos, std::string &result);

	// AppStateListener
	void OnGameContextChanging() override;
	void OnGameContextChanged() override;
};
