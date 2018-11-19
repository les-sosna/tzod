#include "Campaign.h"
#include "Game.h"
#include "GetFileName.h"
#include "gui.h"
#include "LuaConsole.h"
#include "MainMenu.h"
#include "NavStack.h"
#include "SelectMapDlg.h"
#include "Settings.h"
#include "SinglePlayer.h"
#include "Widgets.h"
#include "inc/shell/Config.h"
#include "inc/shell/Desktop.h"
#include "inc/shell/Profiler.h"
#include <as/AppConstants.h>
#include <as/AppController.h>
#include <as/AppState.h>
#include <cbind/ConfigBinding.h>
#include <ctx/EditorContext.h>
#include <editor/Editor.h>
#include <editor/MapSettings.h>
#include <gc/World.h>
#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <plat/Input.h>
#include <plat/Keys.h>
#include <ui/Button.h>
#include <ui/Console.h>
#include <plat/ConsoleBuffer.h>
#include <ui/DataSource.h>
#include <ui/InputContext.h>
#include <ui/GuiManager.h>
#include <ui/LayoutContext.h>
#include <ui/Text.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <functional>


Desktop::Desktop(UI::TimeStepManager &manager,
                 TextureManager &texman,
                 AppState &appState,
                 AppConfig &appConfig,
                 AppController &appController,
                 FS::FileSystem &fs,
                 ShellConfig &conf,
                 LangCache &lang,
                 DMCampaign &dmCampaign,
                 Plat::ConsoleBuffer &logger)
	: Managerful(manager)
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
	, _renderScheme(texman)
	, _worldView(texman, _renderScheme)
	, _mapCollection(*fs.GetFileSystem(DIR_MAPS))
{
	using namespace std::placeholders;
	using namespace UI::DataSourceAliases;

	_tierTitle = std::make_shared<UI::Text>();
	_tierTitle->SetAlign(alignTextCC);
	_tierTitle->SetFont("font_default");
	AddFront(_tierTitle);

	_background = std::make_shared<UI::Rectangle>();
	_background->SetTexture("gui_splash");
	_background->SetTextureStretchMode(UI::StretchMode::Fill);
	_background->SetDrawBorder(false);
	_background->SetBackColor(0xff505050_rgba);
	AddFront(_background);

	_con = UI::Console::Create(this, manager, texman, 10, 0, 100, 100, &_logger);
	_con->eventOnSendCommand = std::bind(&Desktop::OnCommand, this, _1);
	_con->eventOnRequestCompleteCommand = std::bind(&Desktop::OnCompleteCommand, this, _1, _2, _3);
	_con->SetVisible(false);
	_con->SetTopMost(true);
	SpriteColor colors[] = {0xffffffff, 0xffff7fff};
	_con->SetColors(colors, sizeof(colors) / sizeof(colors[0]));
	_con->SetHistory(&_history);

	_fps = std::make_shared<FpsCounter>(manager, alignTextLB, GetAppState());
	AddFront(_fps);
	_conf.d_showfps.eventChange = std::bind(&Desktop::OnChangeShowFps, this);
	OnChangeShowFps();

	if( _conf.d_graph.Get() )
	{
		float xx = 200;
		float yy = 3;
		float hh = 50;
		for( size_t i = 0; i < CounterBase::GetMarkerCountStatic(); ++i )
		{
			auto os = std::make_shared<Oscilloscope>();
			os->Move(xx, yy);
			os->Resize(400, hh);
			os->SetRange(-1/15.0f, 1/15.0f);
			os->SetTitle(CounterBase::GetMarkerInfoStatic(i).title);
			os->SetTopMost(true);
			AddFront(os);
			CounterBase::SetMarkerCallbackStatic(i, std::bind(&Oscilloscope::Push, os, std::ref(texman), std::placeholders::_1));
			yy += hh+5;
		}
	}

	_pauseButton = std::make_shared<UI::Button>();
	_pauseButton->SetBackground("ui/pause");
	_pauseButton->AlignToBackground(texman);
	_pauseButton->SetTopMost(true);
	_pauseButton->eventClick = [=]()
	{
		if (!_navStack->GetNavFront())
		{
			ShowMainMenu();
		}
		else
		{
			_navStack->PopNavStack();
			UpdateFocus();
		}
	};
	AddFront(_pauseButton);

	_navStack = std::make_shared<NavStack>(manager);
	_navStack->SetSpacing(_conf.ui_nav_spacing.GetFloat());
	AddFront(_navStack);

	OnGameContextChanged();
}

Desktop::~Desktop()
{
	_conf.d_showfps.eventChange = nullptr;
}

bool Desktop::GetEditorMode() const
{
	return _editor && _editor->GetVisible();
}

void Desktop::SetEditorMode(bool editorMode)
{
	if (!editorMode || _appController.GetEditorModeAvailable())
	{
		_appController.SetEditorMode(GetAppState(), editorMode);
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

void Desktop::OnCloseChild(std::shared_ptr<UI::Window> child)
{
	_navStack->PopNavStack(child.get());
	UpdateFocus();
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
	auto dlg = std::make_shared<NewCampaignDlg>(_fs, _lang);
	dlg->eventCampaignSelected = [this](auto sender, std::string_view name)
	{
		OnCloseChild(sender);
		if( !name.empty() )
		{
			try
			{
//				script_exec_file(_globL.get(), _fs, ("campaign/" + name + ".lua").c_str());
				throw std::logic_error("not implemented");
			}
			catch( const std::exception &e )
			{
				_logger.WriteLine(1, e.what());
				ShowConsole(true);
			}
		}
	};
	_navStack->PushNavStack(dlg);
	UpdateFocus();
}

void Desktop::OnSinglePlayer()
{
	if (_navStack->IsOnStack<NewGameDlg>() || _navStack->IsOnStack<SinglePlayer>())
		return;

//	if (_navStack->IsOnTop<SettingsDlg>())
//		_navStack->PopNavStack();

	if (_dmCampaign.tiers.GetSize() > 0)
	{
		auto dlg = std::make_shared<SinglePlayer>(_worldView, _fs, _appConfig, _conf, _dmCampaign, _appController.GetWorldCache());
		dlg->eventSelectMap = [this](auto sender, int index)
		{
			_conf.sp_map.SetInt(index);
			OnCloseChild(sender);
			try
			{
				int currentTier = GetCurrentTier(_conf, _dmCampaign);
				int currentMap = GetCurrentMap(_conf, _dmCampaign);
				_appController.StartDMCampaignMap(GetAppState(), _appConfig, _dmCampaign, currentTier, currentMap);
				NavigateHome();
			}
			catch (const std::exception &e)
			{
				_logger.Printf(1, "Could not start new game - %s", e.what());
			}
		};
		_navStack->PushNavStack(dlg);
		UpdateFocus();
	}
}

void Desktop::OnSplitScreen()
{
	if (_navStack->IsOnStack<NewGameDlg>() || _navStack->IsOnStack<SinglePlayer>())
		return;

	auto dlg = std::make_shared<NewGameDlg>(_texman, _fs, _conf, _logger, _lang);
	dlg->eventClose = [this](auto sender, int result)
	{
		OnCloseChild(sender);
		if (UI::Dialog::_resultOK == result)
		{
			try
			{
//				_appController.NewGameDM(GetAppState(), _conf.cl_map.Get(), GetDMSettingsFromConfig(_conf));
				NavigateHome();
			}
			catch (const std::exception &e)
			{
				_logger.Printf(1, "Could not start new game - %s", e.what());
			}
		}
	};
	_navStack->PushNavStack(dlg);
	UpdateFocus();
}

void Desktop::OnOpenMap()
{
	auto mapsFolder = _fs.GetFileSystem(DIR_MAPS);
	if (!mapsFolder)
	{
		ShowConsole(true);
		_logger.Printf(1, "Could not open directory '%s'", DIR_MAPS);
		return;
	}

	auto selectMapDlg = std::make_shared<SelectMapDlg>(_worldView, _fs, _conf, _lang, _appController.GetWorldCache(), _mapCollection);
	selectMapDlg->eventMapSelected = [this](std::shared_ptr<SelectMapDlg> sender, unsigned int mapIndex)
	{
		OnCloseChild(sender);
		std::shared_ptr<FS::Stream> stream;
		if (mapIndex != -1)
		{
			auto fileName = std::string(DIR_MAPS).append("/").append(_mapCollection.GetMapName(mapIndex)) + ".map";
			stream = _fs.Open(fileName)->QueryStream();
		}
		// clear the existing context first to prevent memory usage spike
		GetAppState().SetGameContext(nullptr);
		std::unique_ptr<GameContextBase> gc(new EditorContext(_conf.editor.width.GetInt(), _conf.editor.height.GetInt(), stream.get()));
		GetAppState().SetGameContext(std::move(gc));
		NavigateHome();
	};

	_navStack->PushNavStack(selectMapDlg);
	UpdateFocus();
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

		auto fileDlg = std::make_shared<GetFileNameDlg>(param, _lang);
		fileDlg->eventClose = [this](auto sender, int result)
		{
			OnCloseChild(sender);
			auto gameContext = GetAppState().GetGameContext();
			if (UI::Dialog::_resultOK == result && gameContext)
			{
				auto fileName = std::string(DIR_MAPS) + "/" + static_cast<GetFileNameDlg&>(*sender).GetFileName();
				gameContext->GetWorld().Export(*_fs.Open(fileName, FS::ModeWrite)->QueryStream());
				_logger.Printf(0, "map exported: '%s'", fileName.c_str());
			}
		};
		_navStack->PushNavStack(fileDlg);
		UpdateFocus();
	}
}

void Desktop::OnGameSettings()
{
	if (_navStack->IsOnStack<SettingsDlg>())
		return;

	auto dlg = std::make_shared<SettingsDlg>(_texman, _conf, _lang);
	dlg->eventClose = [this](auto sender, int result) {OnCloseChild(sender);};
	_navStack->PushNavStack(dlg);
	UpdateFocus();
}

void Desktop::OnMapSettings()
{
	if (auto gameContext = GetAppState().GetGameContext())
	{
		//ThemeManager themeManager(GetAppState(), _fs, _texman);
		auto dlg = std::make_shared<MapSettingsDlg>(gameContext->GetWorld()/*, themeManager*/, _lang);
		dlg->eventClose = [this](auto sender, int result) {OnCloseChild(sender);};
		_navStack->PushNavStack(dlg);
		UpdateFocus();
	}
}

void Desktop::ShowMainMenu()
{
	MainMenuCommands commands;
	commands.singlePlayer = std::bind(&Desktop::OnSinglePlayer, this);
	commands.splitScreen = std::bind(&Desktop::OnSplitScreen, this);
	commands.openMap = std::bind(&Desktop::OnOpenMap, this);
	commands.exportMap = std::bind(&Desktop::OnExportMap, this);
	commands.gameSettings = std::bind(&Desktop::OnGameSettings, this);
	commands.close = [=]()
	{
		if (GetAppState().GetGameContext()) // do not return to nothing
		{
			NavigateHome();
		}
	};
	_navStack->PushNavStack(std::make_shared<MainMenuDlg>(_lang, std::move(commands)));
	UpdateFocus();
}

void Desktop::UpdateFocus()
{
	bool hasGameContext = !!GetAppState().GetGameContext();
	bool showingMainMenu = _navStack->IsOnTop<MainMenuDlg>();

	_pauseButton->SetVisible(hasGameContext || !showingMainMenu);

	if (_con->GetVisible())
	{
		SetFocus(_con);
	}
	else if(_navStack->GetNavFront())
	{
		SetFocus(_navStack);
	}
	else if (_editor)
	{
		SetFocus(_editor);
	}
	else
	{
		SetFocus(_game); // may be null
	}
}

void Desktop::NavigateHome()
{
	while (auto wnd = _navStack->GetNavFront())
	{
		_navStack->PopNavStack(wnd.get());
	}
	UpdateFocus();
}

bool Desktop::OnKeyPressed(UI::InputContext &ic, Plat::Key key)
{
	switch( key )
	{
	case Plat::Key::GraveAccent: // '~'
		_con->SetVisible(!_con->GetVisible());
		// on show do not grab focus yet - wait until the key released
		if (!_con->GetVisible())
		{
			UpdateFocus();
		}
		break;

	case Plat::Key::Escape:
	case Plat::Key::GamepadMenu:
		if( GetFocus() == _con )
		{
			_con->SetVisible(false);
			UpdateFocus();
		}
		else if (!_navStack->GetNavFront())
		{
			ShowMainMenu();
		}
		else
		{
			return false;
		}
		break;

	case Plat::Key::F2:
		OnSinglePlayer();
		break;

	case Plat::Key::F12:
		OnGameSettings();
		break;

	case Plat::Key::F8:
		OnMapSettings();
		break;

	case Plat::Key::F5:
	case Plat::Key::GamepadView:
		if (!_navStack->GetNavFront())
			SetEditorMode(!GetEditorMode());
		break;

	case Plat::Key::F10:
		if (_conf.d_artistmode.Get())
		{
			_texman.LoadPackage(ParsePackage(FILE_TEXTURES, _fs.Open(FILE_TEXTURES)->QueryMap(), _fs));
		}
		break;

	default:
		return false;
	}

	return true;
}

void Desktop::OnKeyReleased(UI::InputContext &ic, Plat::Key key)
{
	if (Plat::Key::GraveAccent == key)
	{
		UpdateFocus();
	}
}

bool Desktop::CanNavigate(UI::Navigate navigate, const UI::LayoutContext &lc, const UI::DataContext &dc) const
{
	return UI::Navigate::Back == navigate &&
		_navStack->GetNavFront() && (!_navStack->IsOnTop<MainMenuDlg>() || GetAppState().GetGameContext());
}

void Desktop::OnNavigate(UI::Navigate navigate, UI::NavigationPhase phase, const UI::LayoutContext &lc, const UI::DataContext &dc)
{
	if (UI::NavigationPhase::Completed == phase && UI::Navigate::Back == navigate)
	{
		_navStack->PopNavStack();
		UpdateFocus();
	}
}

FRECT Desktop::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	if (_background.get() == &child)
	{
		float navDepth = _navStack->GetNavigationDepth();
		float transition = 1 - (1 - std::cos(PI * std::min(1.f, navDepth))) / 2;
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
		return UI::CanvasLayout(vec2d{ 1, lc.GetPixelSize().y / lc.GetScale() - 1 }, _fps->GetContentSize(texman, dc, lc.GetScale(), DefaultLayoutConstraints(lc)) / lc.GetScale(), lc.GetScale());
	}
	if (_tierTitle.get() == &child)
	{
		return MakeRectWH(Vec2dFloor(lc.GetPixelSize() / 2), vec2d{});
	}
	return UI::Window::GetChildRect(texman, lc, dc, child);
}

float Desktop::GetChildOpacity(const UI::Window &child) const
{
	if (_tierTitle.get() == &child)
	{
		if (auto gameContext = dynamic_cast<GameContext*>(GetAppState().GetGameContext().get()))
			return std::max(0.f, std::min(1.f, (5 - gameContext->GetWorld().GetTime()) / 3));
		else
			return 0;
	}
	return UI::Window::GetChildOpacity(child);
}

void Desktop::OnChangeShowFps()
{
	_fps->SetVisible(_conf.d_showfps.Get());
}

void Desktop::OnCommand(std::string_view cmd)
{
	if( cmd.empty() )
	{
		return;
	}

	if (!_luaConsole)
		_luaConsole = std::make_unique<LuaConsole>(_logger, _conf, _fs);

	_luaConsole->Exec(cmd);
}

bool Desktop::OnCompleteCommand(std::string_view cmd, int &pos, std::string &result)
{
	if (!_luaConsole)
		_luaConsole = std::make_unique<LuaConsole>(_logger, _conf, _fs);

	return _luaConsole->CompleteCommand(cmd, pos, result);
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
	if (auto gameContext = std::dynamic_pointer_cast<GameContext>(GetAppState().GetGameContext()))
	{
		assert(!_game);

		CampaignControlCommands campaignControlCommands;
		campaignControlCommands.replayCurrent = [this]
		{
			_appController.StartDMCampaignMap(GetAppState(), _appConfig, _dmCampaign, GetCurrentTier(_conf, _dmCampaign), GetCurrentMap(_conf, _dmCampaign));
		};
		campaignControlCommands.playNext = [this]
		{
			int tierIndex = GetCurrentTier(_conf, _dmCampaign);
			int nextMapIndex = (GetCurrentMap(_conf, _dmCampaign) + 1) % GetCurrentTierMapCount(_conf, _dmCampaign);
			if (nextMapIndex == 0 && IsTierComplete(_appConfig, _dmCampaign, tierIndex))
				tierIndex++;
			_conf.sp_map.SetInt(nextMapIndex);
			_conf.sp_tier.SetInt(tierIndex);
			_appController.StartDMCampaignMap(GetAppState(), _appConfig, _dmCampaign, tierIndex, nextMapIndex);
		};

		_game = std::make_shared<GameLayout>(
			GetTimeStepManager(),
			gameContext,
			_worldView,
			gameContext->GetWorldController(),
			_conf,
			_lang,
			_logger,
			std::move(campaignControlCommands));
		AddBack(_game);

		int currentTier = GetCurrentTier(_conf, _dmCampaign);
		DMCampaignTier tierDesc(&_dmCampaign.tiers.GetTable(currentTier));
		_tierTitle->SetText(ConfBind(tierDesc.title));
	}
	else if (auto editorContext = std::dynamic_pointer_cast<EditorContext>(GetAppState().GetGameContext()))
	{
		assert(!_editor);
		_editor = std::make_shared<EditorLayout>(
			GetTimeStepManager(),
			_texman,
			*editorContext,
			_worldView,
			_conf.editor,
			_lang,
			EditorCommands{ [this] { SetEditorMode(false); } },
			_logger);
		AddBack(_editor);
	}

	if (!GetAppState().GetGameContext())
	{
		ShowMainMenu();
	}

	UpdateFocus();
}
