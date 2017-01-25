#include "Campaign.h"
#include "Editor.h"
#include "Game.h"
#include "GetFileName.h"
#include "gui.h"
#include "MainMenu.h"
#include "MapSettings.h"
#include "NavStack.h"
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
#include <ui/DataSource.h>
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
                 ShellConfig &conf,
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
		if (!_navStack->GetNavFront())
		{
			ShowMainMenu();
		}
		else
		{
			while (auto wnd = _navStack->GetNavFront())
			{
				_navStack->PopNavStack(wnd.get());
			}
		}
	};
	AddFront(_pauseButton);

	_navStack = std::make_shared<NavStack>(manager);
	_navStack->SetSpacing(_conf.ui_spacing.GetFloat());
	AddFront(_navStack);

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
	_navStack->PopNavStack(child.get());
}
/*
static DMSettings GetDMSettingsFromConfig(const ShellConfig &conf)
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
	_navStack->PushNavStack(dlg);
}

void Desktop::OnNewDM()
{
	if (_navStack->IsOnStack<NewGameDlg>() || _navStack->IsOnStack<SinglePlayer>())
		return;

//	if (_navStack->IsOnTop<SettingsDlg>())
//		_navStack->PopNavStack();

	if (_dmCampaign.tiers.GetSize() > 0 &&
		!GetManager().GetInputContext().GetInput().IsKeyPressed(UI::Key::LeftCtrl) &&
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
					while (auto wnd = _navStack->GetNavFront())
					{
						_navStack->PopNavStack(wnd.get());
					}
				}
				catch (const std::exception &e)
				{
					_logger.Printf(1, "Could not start new game - %s", e.what());
				}
			}
		};
		_navStack->PushNavStack(dlg);
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
					while (auto wnd = _navStack->GetNavFront())
					{
						_navStack->PopNavStack(wnd.get());
					}
				}
				catch (const std::exception &e)
				{
					_logger.Printf(1, "Could not start new game - %s", e.what());
				}
			}
		};
		_navStack->PushNavStack(dlg);
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
			while (auto wnd = _navStack->GetNavFront())
			{
				_navStack->PopNavStack(wnd.get());
			}
		}
	};
	_navStack->PushNavStack(dlg);
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
			while (auto wnd = _navStack->GetNavFront())
			{
				_navStack->PopNavStack(wnd.get());
			}
		}
	};
	_navStack->PushNavStack(fileDlg);
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
		_navStack->PushNavStack(fileDlg);
	}
}

void Desktop::OnGameSettings()
{
	auto dlg = std::make_shared<SettingsDlg>(GetManager(), _texman, _conf, _lang);
	dlg->eventClose = [this](auto sender, int result) {OnCloseChild(sender, result);};
	_navStack->PushNavStack(dlg);
}

void Desktop::OnMapSettings()
{
	if (auto gameContext = GetAppState().GetGameContext())
	{
		//ThemeManager themeManager(GetAppState(), _fs, _texman);
		auto dlg = std::make_shared<MapSettingsDlg>(GetManager(), _texman, gameContext->GetWorld()/*, themeManager*/, _lang);
		dlg->eventClose = [this](auto sender, int result) {OnCloseChild(sender, result);};
		_navStack->PushNavStack(dlg);
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
			while (auto wnd = _navStack->GetNavFront())
			{
				_navStack->PopNavStack(wnd.get());
			}
		}
	};
	_navStack->PushNavStack(std::make_shared<MainMenuDlg>(GetManager(), _texman, _lang, std::move(commands)));
}

void Desktop::UpdateFocus()
{
	if (_con->GetVisible())
	{
		SetFocus(_con);
	}
	else if(_navStack->GetNavFront())
	{
		SetFocus(_navStack);
	}
	else
	{
		SetFocus(_game); // may be null
	}
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
			if (!_navStack->GetNavFront())
			{
				ShowMainMenu();
			}
			else if(!_navStack->IsOnTop<MainMenuDlg>() || GetAppState().GetGameContext())
			{
				_navStack->PopNavStack();
			}
			if (!_navStack->GetNavFront() && !GetAppState().GetGameContext())
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
		if (!_navStack->GetNavFront())
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
	if (_editor.get() == &child || _game.get() == &child || _navStack.get() == &child)
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

		CampaignControlCommands campaignControlCommands;
		campaignControlCommands.replayCurrent = [this, weakThis = std::weak_ptr<Window>(shared_from_this())]
		{
			if (!weakThis.expired())
				_appController.StartMapDMCampaign(GetAppState(), _appConfig, _dmCampaign, GetCurrentTier(_conf, _dmCampaign), GetCurrentMap(_conf, _dmCampaign));
		};
		campaignControlCommands.playNext = [this, weakThis = std::weak_ptr<Window>(shared_from_this())]
		{
			if (!weakThis.expired())
			{
				int tierIndex = GetCurrentTier(_conf, _dmCampaign);
				int nextMapIndex = (GetCurrentMap(_conf, _dmCampaign) + 1) % GetCurrentTierMapCount(_conf, _dmCampaign);
				if (nextMapIndex == 0 && IsTierComplete(_appConfig, _dmCampaign, tierIndex))
					tierIndex++;
				_conf.sp_map.SetInt(nextMapIndex);
				_conf.sp_tier.SetInt(tierIndex);
				_appController.StartMapDMCampaign(GetAppState(), _appConfig, _dmCampaign, tierIndex, nextMapIndex);
			}
		};

		_game = std::make_shared<GameLayout>(
			GetManager(),
			_texman,
			*gameContext,
			_worldView,
			gameContext->GetWorldController(),
			_conf,
			_lang,
			_logger,
			std::move(campaignControlCommands));
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
