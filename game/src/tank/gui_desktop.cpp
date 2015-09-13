// gui_desktop.cpp

#include "gui_campaign.h"
#include "gui_desktop.h"
#include "gui_editor.h"
#include "gui_game.h"
#include "gui_settings.h"
#include "gui_mainmenu.h"
#include "gui_widgets.h"
#include "gui.h"

#include <app/AppController.h>
#include <app/EditorContext.h>
#include <app/GameContext.h>

#include "Config.h"
#include "core/Profiler.h"
#include "core/Debug.h"

//#include "network/TankClient.h"

#include <app/AppState.h>
#include <gc/World.h>
#include <loc/Language.h>
#include <fs/FileSystem.h>
//#include <script/script.h>
#include <ui/Console.h>
#include <ui/ConsoleBuffer.h>
#include <ui/GuiManager.h>
#include <video/TextureManager.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <GLFW/glfw3.h>
#include <functional>


static CounterBase counterDt("dt", "dt, ms");


void LuaStateDeleter::operator()(lua_State *L)
{
	lua_close(L);
}


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

Desktop::Desktop(LayoutManager* manager,
				 AppState &appState,
                 AppController &appController,
				 FS::FileSystem &fs,
				 std::function<void()> exitCommand)
  : Window(NULL, manager)
  , AppStateListener(appState)
  , _appController(appController)
  , _fs(fs)
  , _exitCommand(std::move(exitCommand))
  , _globL(luaL_newstate())
  , _font(GetManager().GetTextureManager().FindSprite("font_default"))
  , _nModalPopups(0)
  , _renderScheme(GetManager().GetTextureManager())
  , _worldView(GetManager().GetTextureManager(), _renderScheme)
{
	using namespace std::placeholders;

	if (!_globL)
		throw std::bad_alloc();

	SetTexture("ui/window", false);
	
	_con = Console::Create(this, 10, 0, 100, 100, &GetConsole());
	_con->eventOnSendCommand = std::bind(&Desktop::OnCommand, this, _1);
	_con->eventOnRequestCompleteCommand = std::bind(&Desktop::OnCompleteCommand, this, _1, _2, _3);
	_con->SetVisible(false);
	_con->SetTopMost(true);
	SpriteColor colors[] = {0xffffffff, 0xffff7fff};
	_con->SetColors(colors, sizeof(colors) / sizeof(colors[0]));
	_con->SetHistory(&_history);

	_fps = new FpsCounter(this, 0, 0, alignTextLB, GetAppState());
	g_conf.ui_showfps.eventChange = std::bind(&Desktop::OnChangeShowFps, this);
	OnChangeShowFps();

	MainMenuCommands commands;
	commands.newCampaign = [this]() { OnNewCampaign(); };
	commands.newDM = std::bind(&Desktop::OnNewDM, this);
	commands.newMap = std::bind(&Desktop::OnNewMap, this);
	commands.openMap = std::bind(&Desktop::OnOpenMap, this, _1);
	commands.exit = _exitCommand;
	_mainMenu = new MainMenuDlg(this, _fs, std::move(commands));
	_nModalPopups++;

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
            CounterBase::SetMarkerCallbackStatic(i, std::bind(&Oscilloscope::Push, os, std::placeholders::_1));
			yy += hh+5;
		}
	}
    
    SetTimeStep(true);
}

Desktop::~Desktop()
{    
	g_conf.ui_showfps.eventChange = nullptr;
}
    
void Desktop::OnTimeStep(float dt)
{
	dt *= g_conf.sv_speed.GetFloat() / 100.0f;
    
//	if( !IsGamePaused() || !IsPauseSupported() )
	if (GameContextBase *gc = GetAppState().GetGameContext())
	{
		assert(dt >= 0);
		counterDt.Push(dt);
        
        _defaultCamera.HandleMovement(GetManager().GetInput(),
									  gc->GetWorld()._sx,
									  gc->GetWorld()._sy,
									  (float) GetWidth(),
									  (float) GetHeight());
	}
}

bool Desktop::GetEditorMode() const
{
	return _editor && _editor->GetVisible();
}

void Desktop::SetEditorMode(bool editorMode)
{
	if( _editor )
	{
		_editor->SetVisible(editorMode);
		if( _game )
			_game->SetVisible(!editorMode);
		if( editorMode && !_con->GetVisible() )
		{
			GetManager().SetFocusWnd(_editor);
		}
	}
}
    
bool Desktop::IsGamePaused() const
{
	return _nModalPopups > 0 || _editor->GetVisible(); //  || _world._limitHit
}

void Desktop::ShowConsole(bool show)
{
	_con->SetVisible(show);
}

void Desktop::OnCloseChild(int result)
{
    _nModalPopups--;
}
	
static PlayerDesc GetPlayerDescFromConf(const ConfPlayerBase &p)
{
	PlayerDesc result;
	result.nick = p.nick.Get();
	result.cls = p.platform_class.Get();
	result.skin = p.skin.Get();
	result.team = p.team.GetInt();
	return result;
}

static DMSettings GetDMSettingsFromConfig()
{
	DMSettings settings;
	
	for( size_t i = 0; i < g_conf.dm_players.GetSize(); ++i )
	{
		ConfPlayerLocal p(g_conf.dm_players.GetAt(i)->AsTable());
		settings.players.push_back(GetPlayerDescFromConf(p));
	}
	
	for( size_t i = 0; i < g_conf.dm_bots.GetSize(); ++i )
	{
		ConfPlayerAI p(g_conf.dm_bots.GetAt(i)->AsTable());
		settings.bots.push_back(GetPlayerDescFromConf(p));
	}
	
	return settings;
}
	
void Desktop::OnNewCampaign()
{
	_nModalPopups++;
	NewCampaignDlg *dlg = new NewCampaignDlg(this, _fs);
	dlg->eventCampaignSelected = [this,dlg](std::string name)
	{
		dlg->Destroy();
		if( !name.empty() )
		{
			OnCloseChild(Dialog::_resultOK);

			g_conf.ui_showmsg.Set(true);
            try
            {
//                script_exec_file(_globL.get(), _fs, ("campaign/" + name + ".lua").c_str());
                throw std::logic_error("not implemented");
            }
            catch( const std::exception &e )
            {
                GetConsole().WriteLine(1, e.what());
                ShowConsole(true);
            }
		}
		else
		{
			OnCloseChild(Dialog::_resultCancel);
		}
	};
}

void Desktop::OnNewDM()
{
	_nModalPopups++;
	auto dlg = new NewGameDlg(this, _fs);
	dlg->eventClose = [this](int result)
	{
		OnCloseChild(result);
		if (Dialog::_resultOK == result)
		{
			try
			{
                _appController.NewGameDM(GetAppState(), g_conf.cl_map.Get(), GetDMSettingsFromConfig());
				ShowMainMenu(false);
			}
			catch( const std::exception &e )
			{
				TRACE("could not load map - %s", e.what());
			}
		}
	};
}

void Desktop::OnNewMap()
{
	_nModalPopups++;
	auto dlg = new NewMapDlg(this);
	dlg->eventClose = [&](int result)
	{
		OnCloseChild(result);
		if (Dialog::_resultOK == result)
		{
			std::unique_ptr<GameContextBase> gc(new EditorContext(g_conf.ed_width.GetFloat(), g_conf.ed_height.GetFloat()));
			GetAppState().SetGameContext(std::move(gc));
			ShowMainMenu(false);
		}
	};
}

void Desktop::OnOpenMap(std::string fileName)
{
	std::unique_ptr<GameContextBase> gc(new EditorContext(*_fs.Open(fileName)->QueryStream()));
	GetAppState().SetGameContext(std::move(gc));
	ShowMainMenu(false);
}
	
void Desktop::ShowMainMenu(bool show)
{
	if (_mainMenu->GetVisible() != show)
	{
		if (_mainMenu->GetVisible())
		{
			_mainMenu->SetVisible(false);
			OnCloseChild(0);
		}
		else
		{
			_mainMenu->SetVisible(true);
			GetManager().SetFocusWnd(_mainMenu);
			_nModalPopups++;
		}
	}
}

bool Desktop::OnRawChar(int c)
{
	Dialog *dlg = nullptr;

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
			GetManager().SetFocusWnd(_con);
		}
		break;

	case GLFW_KEY_ESCAPE:
		if( _con->Contains(GetManager().GetFocusWnd()) )
		{
			_con->SetVisible(false);
		}
		else if( GetAppState().GetGameContext() )
		{
			ShowMainMenu(!_mainMenu->GetVisible());
		}
		break;

	case GLFW_KEY_F2:
		OnNewDM();
		break;

	case GLFW_KEY_F12:
		dlg = new SettingsDlg(this);
        dlg->eventClose = std::bind(&Desktop::OnCloseChild, this, std::placeholders::_1);
        _nModalPopups++;
		break;

	case GLFW_KEY_F5:
        if (0 == _nModalPopups)
            SetEditorMode(!GetEditorMode());
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
	if( _editor )
		_editor->Resize(width, height);
	if( _game )
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
//        SendTextMessage(cmd);
        return;
    }


	if( luaL_loadstring(_globL.get(), exec.c_str()) )
	{
		lua_pop(_globL.get(), 1);

		std::string tmp = "print(";
		tmp += exec;
		tmp += ")";

		if( luaL_loadstring(_globL.get(), tmp.c_str()) )
		{
			lua_pop(_globL.get(), 1);
		}
		else
		{
//			script_exec(_globL.get(), tmp.c_str());
			return;
		}
	}

//	script_exec(_globL.get(), exec.c_str());
}

bool Desktop::OnCompleteCommand(const std::string &cmd, int &pos, std::string &result)
{
	assert(pos >= 0);
	lua_getglobal(_globL.get(), "autocomplete"); // FIXME: can potentially throw
	if( lua_isnil(_globL.get(), -1) )
	{
		lua_pop(_globL.get(), 1);
		GetConsole().WriteLine(1, "There was no autocomplete module loaded");
		return false;
	}
	lua_pushlstring(_globL.get(), cmd.substr(0, pos).c_str(), pos);
	if( lua_pcall(_globL.get(), 1, 1, 0) )
	{
		GetConsole().WriteLine(1, lua_tostring(_globL.get(), -1));
        lua_pop(_globL.get(), 1); // pop error message
	}
	else
	{
		const char *str = lua_tostring(_globL.get(), -1);
		std::string insert = str ? str : "";

		result = cmd.substr(0, pos) + insert + cmd.substr(pos);
		pos += insert.length();

		if( !result.empty() && result[0] != '/' )
		{
			result = std::string("/") + result;
			++pos;
		}
	}
	lua_pop(_globL.get(), 1); // pop result or error message
	return true;
}
	
void Desktop::OnGameContextChanging()
{
	if (_game)
	{
		_game->Destroy();
		_game = nullptr;
	}
	
	if (_editor)
	{
		_editor->Destroy();
		_editor = nullptr;
	}
}

void Desktop::OnGameContextChanged()
{
	if (auto *gc = dynamic_cast<GameContext*>(GetAppState().GetGameContext()))
	{
		_game = new GameLayout(this,
							   *gc,
							   _worldView,
							   gc->GetWorldController(),
							   _defaultCamera);
		_game->Resize(GetWidth(), GetHeight());
		_game->BringToBack();
		
		SetEditorMode(false);
	}
	
	if (auto *gc = dynamic_cast<EditorContext*>(GetAppState().GetGameContext()))
	{
		_editor = new EditorLayout(this, gc->GetWorld(), _worldView, _defaultCamera, _globL.get());
		_editor->Resize(GetWidth(), GetHeight());
		_editor->BringToBack();
		_editor->SetVisible(false);

		SetEditorMode(true);
	}
	
	if (!GetAppState().GetGameContext())
		ShowMainMenu(true);
}
	
} // namespace UI
