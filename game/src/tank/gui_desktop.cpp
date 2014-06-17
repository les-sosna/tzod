// gui_desktop.cpp

#include "globals.h"
#include "gui_widgets.h"
#include "gui_desktop.h"
#include "gui_editor.h"
#include "gui_game.h"
#include "gui_settings.h"
#include "gui_mainmenu.h"
#include "gui.h"

#include "video/TextureManager.h"

#include "config/Config.h"
#include "config/Language.h"
#include "core/Profiler.h"
#include "gc/World.h"

//#include "network/TankClient.h"

#include "script.h"

#include <Console.h>
#include <ConsoleBuffer.h>
#include <GuiManager.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}


UI::ConsoleBuffer& GetConsole();


#include <GLFW/glfw3.h>


static CounterBase counterDt("dt", "dt, ms");


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

void Desktop::MyConsoleHistory::Enter(const std::string &str)
{
	g_conf.con_history.PushBack(ConfVar::typeString)->AsStr()->Set(str);
	while( (signed) g_conf.con_history.GetSize() > g_conf.con_maxhistory.GetInt() )
	{
		g_conf.con_history.PopFront();
	}
}

size_t Desktop::MyConsoleHistory::GetItemCount() const
{
	return g_conf.con_history.GetSize();
}

const std::string& Desktop::MyConsoleHistory::GetItem(size_t index) const
{
	return g_conf.con_history.GetStr(index, "")->Get();
}

Desktop::Desktop(LayoutManager* manager, World &world, WorldController &worldController, AIManager &aiMgr)
  : Window(NULL, manager)
  , _aiMgr(aiMgr)
  , _font(GetManager()->GetTextureManager().FindSprite("font_default"))
  , _nModalPopups(0)
  , _world(world)
  , _worldView(*g_render, GetManager()->GetTextureManager())
  , _worldController(worldController)
{
	SetTexture("ui/window", false);

	_editor = new EditorLayout(this, _world, _worldView, _defaultCamera);
	_editor->SetVisible(false);
    
	_game = new GameLayout(this, _world, _worldView, _worldController, _inputMgr, _defaultCamera);

	_con = Console::Create(this, 10, 0, 100, 100, &GetConsole());
	_con->eventOnSendCommand = std::bind( &Desktop::OnCommand, this, std::placeholders::_1 );
	_con->eventOnRequestCompleteCommand = std::bind( &Desktop::OnCompleteCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
	_con->SetVisible(false);
	_con->SetTopMost(true);
	SpriteColor colors[] = {0xffffffff, 0xffff7fff};
	_con->SetColors(colors, sizeof(colors) / sizeof(colors[0]));
	_con->SetHistory(&_history);

	_fps = new FpsCounter(this, 0, 0, alignTextLB, _world);
	g_conf.ui_showfps.eventChange = std::bind(&Desktop::OnChangeShowFps, this);
	OnChangeShowFps();


	if( g_conf.dbg_graph.Get() )
	{
		float xx = 200;
		float yy = 3;
		float hh = 50;
		for( size_t i = 0; i < CounterBase::GetMarkerCountStatic(); ++i )
		{
			Oscilloscope *os = new Oscilloscope(this, xx, yy);
			os->Resize(400, hh);
			os->SetRange(-1/15.0f, 1/15.0f);
			os->SetTitle(CounterBase::GetMarkerInfoStatic(i).title);
			CounterBase::SetMarkerCallbackStatic(i, CreateDelegate(&Oscilloscope::Push, os));
			yy += hh+5;
		}
	}
    
	OnRawChar(GLFW_KEY_ESCAPE); // to invoke main menu dialog
    SetTimeStep(true);
}

Desktop::~Desktop()
{    
	g_conf.ui_showfps.eventChange = nullptr;
}
    
void Desktop::OnTimeStep(float dt)
{
	dt *= g_conf.sv_speed.GetFloat() / 100.0f;
    
//	if( g_client && (!IsGamePaused() || !g_client->SupportPause()) )
	{
		assert(dt >= 0);
		counterDt.Push(dt);
        
        _defaultCamera.HandleMovement(_world._sx, _world._sy, (float) GetWidth(), (float) GetHeight());
	}
}

void Desktop::SetEditorMode(bool editorMode)
{
	_editor->SetVisible(editorMode);
    _game->SetVisible(!editorMode);
    _world.PauseSound(IsGamePaused());
	if( editorMode && !_con->GetVisible() )
	{
		GetManager()->SetFocusWnd(_editor);
	}
}
    
bool Desktop::IsGamePaused() const
{
    return _nModalPopups > 0 || _world._limitHit || _editor->GetVisible();
}

void Desktop::ShowConsole(bool show)
{
	_con->SetVisible(show);
}

void Desktop::OnCloseChild(int result)
{
	SetDrawBackground(false);
    _nModalPopups--;
    _world.PauseSound(IsGamePaused());
}

bool Desktop::OnRawChar(int c)
{
	Dialog *dlg = NULL;

	switch( c )
	{
	case GLFW_KEY_GRAVE_ACCENT: // '~'
		if( _con->GetVisible() )
		{
			_con->SetVisible(false);
		}
		else
		{
			_con->SetVisible(true);
			GetManager()->SetFocusWnd(_con);
		}
		break;

	case GLFW_KEY_ESCAPE:
		if( _con->Contains(GetManager()->GetFocusWnd()) )
		{
			_con->SetVisible(false);
		}
		else
		{
			dlg = new MainMenuDlg(this, _world, _inputMgr, _aiMgr);
			SetDrawBackground(true);
			dlg->eventClose = std::bind(&Desktop::OnCloseChild, this, std::placeholders::_1);
            _nModalPopups++;
		}
		break;

	case GLFW_KEY_F2:
		dlg = new NewGameDlg(this, _world, _inputMgr, _aiMgr);
		SetDrawBackground(true);
        dlg->eventClose = std::bind(&Desktop::OnCloseChild, this, std::placeholders::_1);
        _nModalPopups++;
		break;

	case GLFW_KEY_F12:
		dlg = new SettingsDlg(this, _world);
		SetDrawBackground(true);
        dlg->eventClose = std::bind(&Desktop::OnCloseChild, this, std::placeholders::_1);
        _nModalPopups++;
		break;

	case GLFW_KEY_F5:
        if (0 == _nModalPopups)
            SetEditorMode(!_editor->GetVisible());
		break;

	case GLFW_KEY_F8:
		if( _editor->GetVisible() ) // TODO: move to editor layout
		{
			dlg = new MapSettingsDlg(this, _world);
			SetDrawBackground(true);
			dlg->eventClose = std::bind(&Desktop::OnCloseChild, this, std::placeholders::_1);
            _nModalPopups++;
		}
		break;

	default:
		return false;
	}

	return true;
}

bool Desktop::OnFocus(bool focus)
{
	return true;
}

void Desktop::OnSize(float width, float height)
{
	_editor->Resize(width, height);
    _game->Resize(width, height);
	_con->Resize(width - 20, floorf(height * 0.5f + 0.5f));
	_fps->Move(1, height - 1);
}

void Desktop::OnChangeShowFps()
{
	_fps->SetVisible(g_conf.ui_showfps.Get());
}

void Desktop::OnCommand(const std::string &cmd)
{
	if( cmd.empty() )
	{
		return;
	}

	std::string exec;

    if( cmd[0] == '/' )
    {
        exec = cmd.substr(1); // cut off the first symbol and execute cmd
    }
    else
    {
//        dynamic_cast<TankClient*>(g_client)->SendTextMessage(cmd);
        return;
    }


	if( luaL_loadstring(g_env.L, exec.c_str()) )
	{
		lua_pop(g_env.L, 1);

		std::string tmp = "print(";
		tmp += exec;
		tmp += ")";

		if( luaL_loadstring(g_env.L, tmp.c_str()) )
		{
			lua_pop(g_env.L, 1);
		}
		else
		{
			script_exec(g_env.L, tmp.c_str());
			return;
		}
	}

	script_exec(g_env.L, exec.c_str());
}

bool Desktop::OnCompleteCommand(const std::string &cmd, int &pos, std::string &result)
{
	assert(pos >= 0);
	lua_getglobal(g_env.L, "autocomplete");
	if( lua_isnil(g_env.L, -1) )
	{
		lua_pop(g_env.L, 1);
		GetConsole().WriteLine(1, "There was no autocomplete module loaded");
		return false;
	}
	lua_pushlstring(g_env.L, cmd.substr(0, pos).c_str(), pos);
	if( lua_pcall(g_env.L, 1, 1, 0) )
	{
		GetConsole().WriteLine(1, lua_tostring(g_env.L, -1));
        lua_pop(g_env.L, 1); // pop error message
	}
	else
	{
		const char *str = lua_tostring(g_env.L, -1);
		std::string insert = str ? str : "";

		result = cmd.substr(0, pos) + insert + cmd.substr(pos);
		pos += insert.length();

		if( !result.empty() && result[0] != '/' )
		{
			result = std::string("/") + result;
			++pos;
		}
	}
	lua_pop(g_env.L, 1); // pop result or error message
	return true;
}
    
} // end of namespace UI

// end of file
