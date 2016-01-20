#include "Campaign.h"
#include "Desktop.h"
#include "Editor.h"
#include "Game.h"
#include "gui.h"
#include "MainMenu.h"
#include "NewMap.h"
#include "Settings.h"
#include "Widgets.h"
#include "inc/shell/Config.h"
#include "inc/shell/Profiler.h"

#include <as/AppController.h>
#include <as/AppState.h>
#include <ctx/EditorContext.h>
#include <gc/World.h>
#include <fs/FileSystem.h>
#include <loc/Language.h>
//#include <script/script.h>
#include <ui/Button.h>
#include <ui/Console.h>
#include <ui/ConsoleBuffer.h>
#include <ui/GuiManager.h>
#include <ui/Keys.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <functional>


static CounterBase counterDt("dt", "dt, ms");


void LuaStateDeleter::operator()(lua_State *L)
{
	lua_close(L);
}


Desktop::Desktop(UI::LayoutManager* manager,
                 AppState &appState,
                 AppController &appController,
                 FS::FileSystem &fs,
                 ConfCache &conf,
                 LangCache &lang,
                 UI::ConsoleBuffer &logger)
  : Window(nullptr, manager)
  , AppStateListener(appState)
  , _history(conf)
  , _appController(appController)
  , _fs(fs)
  , _conf(conf)
  , _lang(lang)
  , _logger(logger)
  , _globL(luaL_newstate())
  , _renderScheme(GetManager().GetTextureManager())
  , _worldView(GetManager().GetTextureManager(), _renderScheme)
{
	using namespace std::placeholders;

	if (!_globL)
		throw std::bad_alloc();

	SetTexture("ui/window", false);

	_con = UI::Console::Create(this, 10, 0, 100, 100, &_logger);
	_con->eventOnSendCommand = std::bind(&Desktop::OnCommand, this, _1);
	_con->eventOnRequestCompleteCommand = std::bind(&Desktop::OnCompleteCommand, this, _1, _2, _3);
	_con->SetVisible(false);
	_con->SetTopMost(true);
	SpriteColor colors[] = {0xffffffff, 0xffff7fff};
	_con->SetColors(colors, sizeof(colors) / sizeof(colors[0]));
	_con->SetHistory(&_history);

	_fps = new FpsCounter(this, 0, 0, alignTextLB, GetAppState());
	_conf.ui_showfps.eventChange = std::bind(&Desktop::OnChangeShowFps, this);
	OnChangeShowFps();

	if( _conf.dbg_graph.Get() )
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

	_pauseButton = UI::ImageButton::Create(this, 0, 0, "ui/pause");
	_pauseButton->SetTopMost(true);
	_pauseButton->eventClick = [=]()
	{
		if (_navStack.empty())
		{
			ShowMainMenu();
		}
	};

	SetTimeStep(true);
	OnGameContextChanged();
}

Desktop::~Desktop()
{
	_conf.ui_showfps.eventChange = nullptr;
}

void Desktop::OnTimeStep(float dt)
{
	dt *= _conf.sv_speed.GetFloat() / 100.0f;

	if (_navTransitionTime > 0)
	{
		_navTransitionTime = std::max(0.f, _navTransitionTime - dt);
		OnSize(GetWidth(), GetHeight());
	}

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
	return !_navStack.empty() || _editor->GetVisible(); //  || _world._limitHit
}

void Desktop::ShowConsole(bool show)
{
	_con->SetVisible(show);
	if (show)
	{
		_con->BringToFront();
	}
}

void Desktop::OnCloseChild(UI::Window *child, int result)
{
	PopNavStack(child);
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

static DMSettings GetDMSettingsFromConfig(const ConfCache &conf)
{
	DMSettings settings;

	for( size_t i = 0; i < conf.dm_players.GetSize(); ++i )
	{
		ConfPlayerLocal p(&conf.dm_players.GetAt(i).AsTable());
		settings.players.push_back(GetPlayerDescFromConf(p));
	}

	for( size_t i = 0; i < conf.dm_bots.GetSize(); ++i )
	{
		ConfPlayerAI p(&conf.dm_bots.GetAt(i).AsTable());
		settings.bots.push_back(GetPlayerDescFromConf(p));
	}

	return settings;
}

void Desktop::OnNewCampaign()
{
	NewCampaignDlg *dlg = new NewCampaignDlg(this, _fs, _lang);
	dlg->eventCampaignSelected = [this,dlg](std::string name)
	{
		if( !name.empty() )
		{
			OnCloseChild(dlg, UI::Dialog::_resultOK);

			_conf.ui_showmsg.Set(true);
            try
            {
//                script_exec_file(_globL.get(), _fs, ("campaign/" + name + ".lua").c_str());
                throw std::logic_error("not implemented");
            }
            catch( const std::exception &e )
            {
                _logger.WriteLine(1, e.what());
                ShowConsole(true);
            }
		}
		else
		{
			OnCloseChild(dlg, UI::Dialog::_resultCancel);
		}
		dlg->Destroy();
	};
	PushNavStack(*dlg);
}

void Desktop::OnNewDM()
{
	for (auto wnd: _navStack)
		if (dynamic_cast<NewGameDlg*>(wnd))
			return;

	if (!_navStack.empty() && dynamic_cast<SettingsDlg*>(_navStack.back()) )
		PopNavStack();

	auto dlg = new NewGameDlg(this, _fs, _conf, _logger, _lang);
	dlg->eventClose = [this, dlg](int result)
	{
		OnCloseChild(dlg, result);
		if (UI::Dialog::_resultOK == result)
		{
			try
			{
				_appController.NewGameDM(GetAppState(), _conf.cl_map.Get(), GetDMSettingsFromConfig(_conf));
				ClearNavStack();
			}
			catch( const std::exception &e )
			{
				_logger.Printf(1, "Could not start new game - %s", e.what());
			}
		}
	};
	PushNavStack(*dlg);
}

void Desktop::OnNewMap()
{
	auto dlg = new NewMapDlg(this, _conf, _lang);
	dlg->eventClose = [=](int result)
	{
		OnCloseChild(dlg, result);
		if (UI::Dialog::_resultOK == result)
		{
			std::unique_ptr<GameContextBase> gc(new EditorContext(_conf.ed_width.GetInt(), _conf.ed_height.GetInt()));
			GetAppState().SetGameContext(std::move(gc));
			ClearNavStack();
		}
	};
	PushNavStack(*dlg);
}

void Desktop::OnOpenMap(std::string fileName)
{
	std::unique_ptr<GameContextBase> gc(new EditorContext(*_fs.Open(fileName)->QueryStream()));
	GetAppState().SetGameContext(std::move(gc));
	ClearNavStack();
}

void Desktop::OnExportMap(std::string fileName)
{
	if (GameContextBase *gameContext = GetAppState().GetGameContext())
	{
		gameContext->GetWorld().Export(*_fs.Open(fileName, FS::ModeWrite)->QueryStream());
		_logger.Printf(0, "map exported: '%s'", fileName.c_str());
//		_conf.cl_map.Set(_fileDlg->GetFileTitle());
	}
}

void Desktop::OnGameSettings()
{
	auto dlg = new SettingsDlg(this, _conf, _lang);
	dlg->eventClose = [=](int result) {OnCloseChild(dlg, result);};
	PushNavStack(*dlg);
}

void Desktop::ShowMainMenu()
{
	using namespace std::placeholders;

	MainMenuCommands commands;
	commands.newCampaign = [this]() { OnNewCampaign(); };
	commands.newDM = std::bind(&Desktop::OnNewDM, this);
	commands.newMap = std::bind(&Desktop::OnNewMap, this);
	commands.openMap = std::bind(&Desktop::OnOpenMap, this, _1);
	commands.exportMap = std::bind(&Desktop::OnExportMap, this, _1);
	commands.gameSettings = std::bind(&Desktop::OnGameSettings, this);
	commands.close = [=]()
	{
		if (GetAppState().GetGameContext()) // do not return to nothing
		{
			ClearNavStack();
		}
	};
	auto mainMenu = new MainMenuDlg(this, _fs, _conf, _lang, _logger, std::move(commands));
	PushNavStack(*mainMenu);
}

void Desktop::ClearNavStack()
{
	for (auto wnd: _navStack)
	{
		wnd->Destroy();
	}
	_navStack.clear();
}

void Desktop::PopNavStack(UI::Window *wnd)
{
	if (wnd)
	{
		_navTransitionStart = GetTransitionTarget();
		_navTransitionTime = _conf.ui_foldtime.GetFloat();

		auto it = std::find(_navStack.begin(), _navStack.end(), wnd);
		assert(_navStack.end() != it);
		_navStack.erase(it);
	}
	else if (!_navStack.empty())
	{
		_navTransitionStart = GetTransitionTarget();
		_navTransitionTime = _conf.ui_foldtime.GetFloat();

		_navStack.back()->Destroy();
		_navStack.pop_back();
	}

	if (!_navStack.empty())
	{
		_navStack.back()->SetEnabled(true);
	}

	OnSize(GetWidth(), GetHeight());
}

void Desktop::PushNavStack(UI::Window &wnd)
{
	_navTransitionStart = GetTransitionTarget();
	_navTransitionTime = _conf.ui_foldtime.GetFloat();

	if (!_navStack.empty())
	{
		_navStack.back()->SetEnabled(false);
	}
	_navStack.push_back(&wnd);
	GetManager().SetFocusWnd(&wnd);
	OnSize(GetWidth(), GetHeight());
}

float Desktop::GetNavStackSize() const
{
	float navStackHeight = 0;
	if (!_navStack.empty())
	{
		for (auto wnd : _navStack)
		{
			navStackHeight += wnd->GetHeight();
		}
		navStackHeight += (float)(_navStack.size() - 1) * _conf.ui_spacing.GetFloat();
	}
	return navStackHeight;
}

float Desktop::GetTransitionTarget() const
{
	return (GetHeight() + (_navStack.empty() ? 0 : _navStack.back()->GetHeight())) / 2 - GetNavStackSize();
}

bool Desktop::OnKeyPressed(UI::Key key)
{
	switch( key )
	{
	case UI::Key::GraveAccent: // '~'
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

	case UI::Key::Escape:
		if( _con->Contains(GetManager().GetFocusWnd()) )
		{
			_con->SetVisible(false);
		}
		else
		{
			if (_navStack.empty())
			{
				ShowMainMenu();
			}
			else if(!IsOnTop<MainMenuDlg>() || GetAppState().GetGameContext())
			{
				PopNavStack();
			}
			if (_navStack.empty() && !GetAppState().GetGameContext())
			{
				ShowMainMenu();
			}
		}
		break;

	case UI::Key::F2:
		OnNewDM();
		break;

	case UI::Key::F12:
		OnGameSettings();
		break;

	case UI::Key::F5:
		if (_navStack.empty())
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

	if (!_navStack.empty())
	{
		float transition = (1 - std::cos(PI * _navTransitionTime / _conf.ui_foldtime.GetFloat())) / 2;
		float top = std::floor(_navTransitionStart * transition + GetTransitionTarget() * (1 - transition));

		for (auto wnd : _navStack)
		{
			wnd->Move(floorf((width - wnd->GetWidth()) / 2), top);
			top += wnd->GetHeight() + _conf.ui_spacing.GetFloat();
		}
	}
}

void Desktop::OnChangeShowFps()
{
	_fps->SetVisible(_conf.ui_showfps.Get());
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
		_logger.WriteLine(1, "There was no autocomplete module loaded");
		return false;
	}
	lua_pushlstring(_globL.get(), cmd.substr(0, pos).c_str(), pos);
	if( lua_pcall(_globL.get(), 1, 1, 0) )
	{
		_logger.WriteLine(1, lua_tostring(_globL.get(), -1));
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
	if (auto *gameContext = dynamic_cast<GameContext*>(GetAppState().GetGameContext()))
	{
		assert(!_game);
		_game = new GameLayout(this,
		                       *gameContext,
		                       _worldView,
		                       gameContext->GetWorldController(),
		                       _defaultCamera,
		                       _conf,
		                       _lang,
		                       _logger);
		_game->Resize(GetWidth(), GetHeight());
		_game->BringToBack();

		SetEditorMode(false);
	}

	if (auto *editorContext = dynamic_cast<EditorContext*>(GetAppState().GetGameContext()))
	{
		assert(!_editor);
		_editor = new EditorLayout(this, editorContext->GetWorld(), _worldView, _defaultCamera, _globL.get(), _conf, _lang, _logger);
		_editor->Resize(GetWidth(), GetHeight());
		_editor->BringToBack();
		_editor->SetVisible(false);

		SetEditorMode(true);
	}

	if (GetAppState().GetGameContext())
	{
		SetDrawBackground(false);
		SetDrawBorder(false);
	}
	else
	{
		SetDrawBackground(true);
		SetDrawBorder(true);
		ShowMainMenu();
	}
}
