#pragma once
#include "DefaultCamera.h"
#include "InputManager.h"
#include "render/RenderScheme.h"
#include "render/WorldView.h"
#include <ui/Window.h>
#include <ui/Console.h>
#include <functional>

struct GameEventSource;
class WorldController;
class AIManager;
class ThemeManager;
namespace FS
{
	class FileSystem;
}

namespace UI
{
    
class EditorLayout;
class GameLayout;
class Console;
class FpsCounter;
class Oscilloscope;


class Desktop
	: public Window
{
	class MyConsoleHistory : public UI::IConsoleHistory
	{
	public:
		virtual void Enter(const std::string &str);
		virtual size_t GetItemCount() const;
		virtual const std::string& GetItem(size_t index) const;
	};

	MyConsoleHistory  _history;
    InputManager _inputMgr;
	AIManager &_aiMgr;
	ThemeManager &_themeManager;
	FS::FileSystem &_fs;
	std::function<void()> _exitCommand;

	EditorLayout *_editor;
    GameLayout   *_game;
	Console      *_con;
	FpsCounter   *_fps;

	size_t _font;
    
    int _nModalPopups;
    
    World &_world;
	RenderScheme _renderScheme;
    WorldView _worldView;
	WorldController &_worldController;
    DefaultCamera _defaultCamera;

    void SetEditorMode(bool editorMode);
    bool IsGamePaused() const;

public:
	Desktop(LayoutManager* manager,
			GameEventSource &gameEventsSource,
			World &_world,
			WorldController &worldController,
			AIManager &aiMgr,
			ThemeManager &themeManager,
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
};


} // end of namespace UI
