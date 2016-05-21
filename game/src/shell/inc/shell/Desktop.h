#pragma once
#include "detail/ConfigConsoleHistory.h"
#include "detail/DefaultCamera.h"
#include <as/AppStateListener.h>
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
	class ButtonBase;
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
	Desktop(UI::LayoutManager &manager,
	        TextureManager &texman,
	        AppState &appState,
	        AppController &appController,
	        FS::FileSystem &fs,
	        ConfCache &conf,
	        LangCache &lang,
	        UI::ConsoleBuffer &logger);
	virtual ~Desktop();

	void ShowConsole(bool show);

protected:
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;
	bool GetNeedsFocus() override;
	void OnSize(float width, float height) override;
	void OnTimeStep(UI::LayoutManager &manager, float dt) override;

private:
	ConfigConsoleHistory  _history;
	TextureManager &_texman;
	AppController &_appController;
	FS::FileSystem &_fs;
	ConfCache &_conf;
	LangCache &_lang;
	UI::ConsoleBuffer &_logger;
	std::unique_ptr<lua_State, LuaStateDeleter> _globL;

	std::shared_ptr<EditorLayout> _editor;
	std::shared_ptr<GameLayout> _game;
	std::shared_ptr<UI::Window> _background;
	std::shared_ptr<UI::Console> _con;
	std::shared_ptr<FpsCounter> _fps;
	std::shared_ptr<UI::ButtonBase> _pauseButton;
	std::vector<std::shared_ptr<UI::Window>> _navStack;
	float _navTransitionTime = 0;
	float _navTransitionStart = 0;
	float _openingTime = 0;

	RenderScheme _renderScheme;
	WorldView _worldView;
	DefaultCamera _defaultCamera;

	bool _initializing = true;

	void OnNewCampaign();
	void OnNewDM();
	void OnNewMap();
	void OnOpenMap();
	void OnExportMap();
	void OnGameSettings();
	void OnMapSettings();
	bool GetEditorMode() const;
	void SetEditorMode(bool editorMode);
	bool IsGamePaused() const;
	void ShowMainMenu();

	void OnChangeShowFps();

	void OnCommand(const std::string &cmd);
	bool OnCompleteCommand(const std::string &cmd, int &pos, std::string &result);

	void OnCloseChild(std::shared_ptr<UI::Window> child, int result);
	void ClearNavStack();
	void PopNavStack(UI::Window *wnd = nullptr);
	void PushNavStack(std::shared_ptr<UI::Window> wnd);

	template <class T>
	bool IsOnTop() const
	{
		return !_navStack.empty() && !!dynamic_cast<T*>(_navStack.back().get());
	}

	float GetNavStackSize() const;
	float GetTransitionTarget() const;

	// AppStateListener
	void OnGameContextChanging() override;
	void OnGameContextChanged() override;
};
