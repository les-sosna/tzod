#pragma once
#include "AppState.h"
#include "DefaultCamera.h"
#include "InputManager.h"
#include <render/RenderScheme.h>
#include <render/WorldView.h>
#include <ui/Window.h>
#include <ui/Console.h>
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
	
class MainMenuDlg;
class EditorLayout;
class GameLayout;
class Console;
class FpsCounter;
class Oscilloscope;


class Desktop
	: public Window
	, private AppStateListener
{
	class MyConsoleHistory : public UI::IConsoleHistory
	{
	public:
		virtual void Enter(const std::string &str);
		virtual size_t GetItemCount() const;
		virtual const std::string& GetItem(size_t index) const;
	};

	MyConsoleHistory  _history;
	FS::FileSystem &_fs;
	std::function<void()> _exitCommand;
	std::unique_ptr<lua_State, LuaStateDeleter> _globL;

	MainMenuDlg  *_mainMenu = nullptr;
	EditorLayout *_editor = nullptr;
    GameLayout   *_game = nullptr;
	Console      *_con = nullptr;
	FpsCounter   *_fps = nullptr;

	size_t _font;
    
    int _nModalPopups;
    
	RenderScheme _renderScheme;
    WorldView _worldView;
    DefaultCamera _defaultCamera;

	void OnNewCampaign();
	void OnNewDM();
	void OnNewMap();
	void OnOpenMap(std::string fileName);
	bool GetEditorMode() const;
    void SetEditorMode(bool editorMode);
    bool IsGamePaused() const;
	void ShowMainMenu(bool show);

public:
	Desktop(LayoutManager* manager,
			AppState &appState,
			FS::FileSystem &fs,
			std::function<void()> exitCommand);
	virtual ~Desktop();

    virtual void OnTimeStep(float dt);

	void ShowConsole(bool show);

	void OnCloseChild(int result);

protected:
	virtual bool OnRawChar(int c);
	virtual bool OnFocus(bool focus);
	virtual void OnSize(float width, float height);

private:
	void OnChangeShowFps();

	void OnCommand(const std::string &cmd);
	bool OnCompleteCommand(const std::string &cmd, int &pos, std::string &result);
	
	// AppStateListener
	virtual void OnGameContextChanging() override;
	virtual void OnGameContextChanged() override;
};


} // end of namespace UI
