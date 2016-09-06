#include "Campaign.h"
#include "Editor.h"
#include "Game.h"
#include "GetFileName.h"
#include "gui.h"
#include "MainMenu.h"
#include "MapSettings.h"
#include "NewMap.h"
#include "Settings.h"
#include "SinglePlayer.h"
#include "Widgets.h"
#include "inc/shell/Config.h"
#include "inc/shell/Desktop.h"
#include "inc/shell/Profiler.h"

#include <as/AppConstants.h>
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
#include <ui/InputContext.h>
#include <ui/GuiManager.h>
#include <ui/Keys.h>
#include <ui/LayoutContext.h>
#include <ui/UIInput.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <functional>


static CounterBase counterDt("dt", "dt, ms");

Desktop::Desktop(UI::LayoutManager &manager,
                 TextureManager &texman,
                 AppState &appState,
                 AppConfig &appConfig,
                 AppController &appController,
                 FS::FileSystem &fs,
                 ConfCache &conf,
                 LangCache &lang,
                 DMCampaign &dmCampaign,
                 UI::ConsoleBuffer &logger)
  : Window(manager)
  , AppStateListener(appState)
  , _history(conf)
  , _texman(texman)
  , _appConfig(appConfig)
  , _appController(appController)
  , _fs(fs)
  , _conf(conf)
  , _lang(lang)
  , _dmCampaign(dmCampaign)
  , _logger(logger)
  , _globL(luaL_newstate())
  , _renderScheme(texman)
  , _worldView(texman, _renderScheme)
{
	using namespace std::placeholders;

	if (!_globL)
		throw std::bad_alloc();

	_background = std::make_shared<UI::Rectangle>(manager);
	_background->SetTexture(texman, "gui_splash", false);
	_background->SetTextureStretchMode(UI::StretchMode::Fill);
	_background->SetDrawBorder(false);
	AddFront(_background);

	_con = UI::Console::Create(this, texman, 10, 0, 100, 100, &_logger);
	_con->eventOnSendCommand = std::bind(&Desktop::OnCommand, this, _1);
	_con->eventOnRequestCompleteCommand = std::bind(&Desktop::OnCompleteCommand, this, _1, _2, _3);
	_con->SetVisible(false);
	_con->SetTopMost(true);
	SpriteColor colors[] = {0xffffffff, 0xffff7fff};
	_con->SetColors(colors, sizeof(colors) / sizeof(colors[0]));
	_con->SetHistory(&_history);

	_fps = std::make_shared<FpsCounter>(manager, texman, 0.f, 0.f, alignTextLB, GetAppState());
	AddFront(_fps);
	_conf.ui_showfps.eventChange = std::bind(&Desktop::OnChangeShowFps, this);
	OnChangeShowFps();

	if( _conf.dbg_graph.Get() )
	{
		float xx = 200;
		float yy = 3;
		float hh = 50;
		for( size_t i = 0; i < CounterBase::GetMarkerCountStatic(); ++i )
		{
			auto os = std::make_shared<Oscilloscope>(manager, texman, xx, yy);
			os->Resize(400, hh);
			os->SetRange(-1/15.0f, 1/15.0f);
			os->SetTitle(CounterBase::GetMarkerInfoStatic(i).title);
			AddFront(os);
			CounterBase::SetMarkerCallbackStatic(i, std::bind(&Oscilloscope::Push, os, std::placeholders::_1));
			yy += hh+5;
		}
	}

	_pauseButton = std::make_shared<UI::Button>(manager, texman);
	_pauseButton->SetBackground(texman, "ui/pause", true);
	_pauseButton->SetTopMost(true);
	_pauseButton->eventClick = [=]()
	{
		if (_navStack.empty())
		{
			ShowMainMenu();
		}
		else
		{
			ClearNavStack();
		}
	};
	AddFront(_pauseButton);

	SetTimeStep(true);
	OnGameContextChanged();

	_initializing = false;
}

Desktop::~Desktop()
{
	_conf.ui_showfps.eventChange = nullptr;
}

void Desktop::OnTimeStep(UI::LayoutManager &manager, float dt)
{
	dt *= _conf.sv_speed.GetFloat() / 100.0f;

	if (_navTransitionTime > 0)
	{
		_navTransitionTime = std::max(0.f, _navTransitionTime - dt);
	}

	if (_openingTime > 0)
	{
		_openingTime = std::max(0.f, _openingTime - dt);
	}

	counterDt.Push(dt);
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
			SetFocus(_editor);
		}
	}
}

void Desktop::ShowConsole(bool show)
{
	_con->SetVisible(show);
	if (show)
	{
		UnlinkChild(*_con);
		AddFront(_con);
	}
}

void Desktop::OnCloseChild(std::shared_ptr<UI::Window> child, int result)
{
	UnlinkChild(*child);
	PopNavStack(child.get());
}
/*
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

	settings.timeLimit = conf.sv_timelimit.GetFloat() * 60;
	settings.fragLimit = conf.sv_fraglimit.GetInt();

	return settings;
}
*/

void Desktop::OnNewCampaign()
{
	auto dlg = std::make_shared<NewCampaignDlg>(GetManager(), _texman, _fs, _lang);
	dlg->eventCampaignSelected = [this](auto sender, std::string name)
	{
		if( !name.empty() )
		{
			OnCloseChild(sender, UI::Dialog::_resultOK);

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
			OnCloseChild(sender, UI::Dialog::_resultCancel);
		}
	};
	PushNavStack(dlg);
}

void Desktop::OnNewDM()
{
	for (auto wnd: _navStack)
		if (dynamic_cast<NewGameDlg*>(wnd.get()) || dynamic_cast<SinglePlayer*>(wnd.get()))
			return;

	if (!_navStack.empty() && dynamic_cast<SettingsDlg*>(_navStack.back().get()) )
		PopNavStack();

	if (!GetManager().GetInputContext().GetInput().IsKeyPressed(UI::Key::LeftCtrl) &&
		!GetManager().GetInputContext().GetInput().IsKeyPressed(UI::Key::RightCtrl))
	{
		auto dlg = std::make_shared<SinglePlayer>(GetManager(), _texman, _worldView, _fs, _appConfig, _conf, _lang, _dmCampaign);
		dlg->eventClose = [this](auto sender, int result)
		{
			OnCloseChild(sender, result);
			if (UI::Dialog::_resultOK == result)
			{
				try
				{
					_appController.StartMapDMCampaign(GetAppState(), _appConfig, _dmCampaign, GetCurrentTier(_conf, _dmCampaign), GetCurrentMap(_conf, _dmCampaign));
					ClearNavStack();
				}
				catch (const std::exception &e)
				{
					_logger.Printf(1, "Could not start new game - %s", e.what());
				}
			}
		};
		PushNavStack(dlg);
	}
	else
	{
		auto dlg = std::make_shared<NewGameDlg>(GetManager(), _texman, _fs, _conf, _logger, _lang);
		dlg->eventClose = [this](auto sender, int result)
		{
			OnCloseChild(sender, result);
			if (UI::Dialog::_resultOK == result)
			{
				try
				{
//					_appController.NewGameDM(GetAppState(), _conf.cl_map.Get(), GetDMSettingsFromConfig(_conf));
					ClearNavStack();
				}
				catch (const std::exception &e)
				{
					_logger.Printf(1, "Could not start new game - %s", e.what());
				}
			}
		};
		PushNavStack(dlg);
	}
}

void Desktop::OnNewMap()
{
	auto dlg = std::make_shared<NewMapDlg>(GetManager(), _texman, _conf, _lang);
	dlg->eventClose = [this](auto sender, int result)
	{
		OnCloseChild(sender, result);
		if (UI::Dialog::_resultOK == result)
		{
			std::unique_ptr<GameContextBase> gc(new EditorContext(_conf.ed_width.GetInt(), _conf.ed_height.GetInt()));
			GetAppState().SetGameContext(std::move(gc));
			ClearNavStack();
		}
	};
	PushNavStack(dlg);
}

void Desktop::OnOpenMap()
{
	if (GetManager().GetInputContext().GetInput().IsKeyPressed(UI::Key::LeftCtrl) ||
		GetManager().GetInputContext().GetInput().IsKeyPressed(UI::Key::RightCtrl))
	{
		OnExportMap();
		return;
	}

	GetFileNameDlg::Params param;
	param.blank = _lang.get_file_name_new_map.Get();
	param.title = _lang.get_file_name_load_map.Get();
	param.folder = _fs.GetFileSystem(DIR_MAPS);
	param.extension = "map";

	if (!param.folder)
	{
		ShowConsole(true);
		_logger.Printf(1, "Could not open directory '%s'", DIR_MAPS);
		return;
	}

	auto fileDlg = std::make_shared<GetFileNameDlg>(GetManager(), _texman, param, _lang);
	fileDlg->eventClose = [this](auto sender, int result)
	{
		OnCloseChild(sender, result);
		if (UI::Dialog::_resultOK == result)
		{
			std::shared_ptr<FS::Stream> stream;
			if (!static_cast<GetFileNameDlg&>(*sender).IsBlank())
			{
				auto fileName = std::string(DIR_MAPS) + "/" + static_cast<GetFileNameDlg&>(*sender).GetFileName();
				stream = _fs.Open(fileName)->QueryStream();
			}
			std::unique_ptr<GameContextBase> gc(new EditorContext(_conf.ed_width.GetInt(), _conf.ed_height.GetInt(), stream.get()));
			GetAppState().SetGameContext(std::move(gc));
			ClearNavStack();
		}
	};
	PushNavStack(fileDlg);
}

void Desktop::OnExportMap()
{
	if (GetAppState().GetGameContext())
	{
		GetFileNameDlg::Params param;
		param.title = _lang.get_file_name_save_map.Get();
		param.folder = _fs.GetFileSystem(DIR_MAPS, true);
		param.extension = "map";

		if (!param.folder)
		{
			ShowConsole(true);
			_logger.Printf(1, "ERROR: Could not open directory '%s'", DIR_MAPS);
			return;
		}

		auto fileDlg = std::make_shared<GetFileNameDlg>(GetManager(), _texman, param, _lang);
		fileDlg->eventClose = [this](auto sender, int result)
		{
			OnCloseChild(sender, result);
			GameContextBase *gameContext = GetAppState().GetGameContext();
			if (UI::Dialog::_resultOK == result && gameContext)
			{
				auto fileName = std::string(DIR_MAPS) + "/" + static_cast<GetFileNameDlg&>(*sender).GetFileName();
				gameContext->GetWorld().Export(*_fs.Open(fileName, FS::ModeWrite)->QueryStream());
				_logger.Printf(0, "map exported: '%s'", fileName.c_str());
			//	_conf.cl_map.Set(fileDlg->GetFileTitle());
			}
		};
		PushNavStack(fileDlg);
	}
}

void Desktop::OnGameSettings()
{
	auto dlg = std::make_shared<SettingsDlg>(GetManager(), _texman, _conf, _lang);
	dlg->eventClose = [this](auto sender, int result) {OnCloseChild(sender, result);};
	PushNavStack(dlg);
}

void Desktop::OnMapSettings()
{
	if (auto gameContext = GetAppState().GetGameContext())
	{
		//ThemeManager themeManager(GetAppState(), _fs, _texman);
		auto dlg = std::make_shared<MapSettingsDlg>(GetManager(), _texman, gameContext->GetWorld()/*, themeManager*/, _lang);
		dlg->eventClose = [this](auto sender, int result) {OnCloseChild(sender, result);};
		PushNavStack(dlg);
	}
}

void Desktop::ShowMainMenu()
{
	MainMenuCommands commands;
	commands.newCampaign = [this]() { OnNewCampaign(); };
	commands.newDM = std::bind(&Desktop::OnNewDM, this);
	commands.newMap = std::bind(&Desktop::OnNewMap, this);
	commands.openMap = std::bind(&Desktop::OnOpenMap, this);
	commands.exportMap = std::bind(&Desktop::OnExportMap, this);
	commands.gameSettings = std::bind(&Desktop::OnGameSettings, this);
	commands.mapSettings = std::bind(&Desktop::OnMapSettings, this);
	commands.close = [=]()
	{
		if (GetAppState().GetGameContext()) // do not return to nothing
		{
			ClearNavStack();
		}
	};
	PushNavStack(std::make_shared<MainMenuDlg>(GetManager(), _texman, _lang, std::move(commands)));
}

void Desktop::ClearNavStack()
{
	for (auto &wnd: _navStack)
	{
		UnlinkChild(*wnd);
	}
	_navStack.clear();
	UpdateFocus();
}

void Desktop::PopNavStack(UI::Window *wnd)
{
	if (wnd)
	{
		_navTransitionStart = GetTransitionTarget();
		_navTransitionTime = _conf.ui_foldtime.GetFloat();

		auto it = std::find_if(_navStack.begin(), _navStack.end(), [wnd](auto &other)
		{
			return other.get() == wnd;
		});
		assert(_navStack.end() != it);
		_navStack.erase(it);
	}
	else if (!_navStack.empty())
	{
		_navTransitionStart = GetTransitionTarget();
		_navTransitionTime = _conf.ui_foldtime.GetFloat();

		UnlinkChild(*_navStack.back());
		_navStack.pop_back();
	}

	if (!_navStack.empty())
	{
		_navStack.back()->SetEnabled(true);
	}

	UpdateFocus();
}

void Desktop::PushNavStack(std::shared_ptr<UI::Window> wnd)
{
	AddFront(wnd);
	_navTransitionStart = GetTransitionTarget();
	if (!_initializing)
	{
		_navTransitionTime = _conf.ui_foldtime.GetFloat();
	}

	if (!_navStack.empty())
	{
		_navStack.back()->SetEnabled(false);
	}
	_navStack.push_back(wnd);
	UpdateFocus();
}

void Desktop::UpdateFocus()
{
	if (_con->GetVisible())
	{
		SetFocus(_con);
	}
	else if(!_navStack.empty())
	{
		SetFocus(_navStack.back());
	}
	else
	{
		SetFocus(_game);
	}
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

bool Desktop::OnKeyPressed(UI::InputContext &ic, UI::Key key)
{
	switch( key )
	{
	case UI::Key::GraveAccent: // '~'
		_con->SetVisible(!_con->GetVisible());
		UpdateFocus();
		break;

	case UI::Key::Escape:
		if( GetFocus() == _con )
		{
			_con->SetVisible(false);
			UpdateFocus();
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

	case UI::Key::F8:
		OnMapSettings();
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

FRECT Desktop::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const
{
	if (_background.get() == &child)
	{
		float transition = (1 - std::cos(PI * _openingTime / _conf.ui_foldtime.GetFloat())) / 2;
		if (GetAppState().GetGameContext())
		{
			transition = 1 - transition;
		}
		return MakeRectWH(vec2d{0, -lc.GetPixelSize().y * transition}, lc.GetPixelSize());
	}
	if (_editor.get() == &child || _game.get() == &child)
	{
		return MakeRectWH(lc.GetPixelSize());
	}
	if (_con.get() == &child)
	{
		return MakeRectRB(Vec2dFloor(vec2d{ 10, 0 } *lc.GetScale()), Vec2dFloor(lc.GetPixelSize().x - 10 * lc.GetScale(), lc.GetPixelSize().y / 2));
	}
	if (_fps.get() == &child)
	{
		return UI::CanvasLayout(vec2d{ 1, lc.GetPixelSize().y / lc.GetScale() - 1 }, _fps->GetContentSize(texman, sc, lc.GetScale()) / lc.GetScale(), lc.GetScale());
	}
	if (!_navStack.empty())
	{
		float transition = (1 - std::cos(PI * _navTransitionTime / _conf.ui_foldtime.GetFloat())) / 2;
		float top = _navTransitionStart * transition + GetTransitionTarget() * (1 - transition);

		for (auto wnd : _navStack)
		{
			vec2d pxWndSize = wnd->GetContentSize(texman, sc, lc.GetScale());
			if (wnd.get() == &child)
			{
				vec2d pxWndOffset = Vec2dFloor((lc.GetPixelSize().x - pxWndSize.x) / 2, top * lc.GetScale());
				return MakeRectWH(pxWndOffset, pxWndSize);
			}
			top += pxWndSize.y / lc.GetScale() + _conf.ui_spacing.GetFloat();
		}
	}
	return UI::Window::GetChildRect(texman, lc, sc, child);
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
		UnlinkChild(*_game);
		_game.reset();
	}

	if (_editor)
	{
		UnlinkChild(*_editor);
		_editor.reset();
	}

	UpdateFocus();
}

void Desktop::OnGameContextChanged()
{
	if (auto *gameContext = dynamic_cast<GameContext*>(GetAppState().GetGameContext()))
	{
		assert(!_game);
		_game = std::make_shared<GameLayout>(
			GetManager(),
			_texman,
			*gameContext,
			_worldView,
			gameContext->GetWorldController(),
			_conf,
			_lang,
			_logger);
		_game->Resize(GetWidth(), GetHeight());
		AddBack(_game);

		SetEditorMode(false);
	}

	if (auto *editorContext = dynamic_cast<EditorContext*>(GetAppState().GetGameContext()))
	{
		assert(!_editor);
		_editor = std::make_shared<EditorLayout>(
			GetManager(),
			_texman,
			*editorContext,
			_worldView,
			_conf,
			_lang,
			_logger);
		_editor->Resize(GetWidth(), GetHeight());
		_editor->SetVisible(false);
		AddBack(_editor);

		SetEditorMode(true);
	}

	if (!_initializing)
	{
		_openingTime = _conf.ui_foldtime.GetFloat();
	}

	if (!GetAppState().GetGameContext())
	{
		ShowMainMenu();
	}

	_pauseButton->SetVisible(!!GetAppState().GetGameContext());

	UpdateFocus();
}
