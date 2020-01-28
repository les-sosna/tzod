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
#include <as/MapCollection.h>
#include <cbind/ConfigBinding.h>
#include <ctx/EditorContext.h>
#include <editor/EditorMain.h>
#include <editor/MapSettings.h>
#include <gc/World.h>
#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <plat/AppWindow.h>
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
                 MapCollection &mapCollection,
                 AppConfig &appConfig,
                 AppController &appController,
                 FS::FileSystem &fs,
                 ShellConfig &conf,
                 LangCache &lang,
                 DMCampaign &dmCampaign,
                 Plat::ConsoleBuffer &logger,
                 Plat::AppWindowCommandClose* cmdClose)
	: Managerful(manager)
	, AppStateListener(appState)
	, _history(conf)
	, _texman(texman)
	, _mapCollection(mapCollection)
	, _appConfig(appConfig)
	, _appController(appController)
	, _fs(fs)
	, _conf(conf)
	, _lang(lang)
	, _dmCampaign(dmCampaign)
	, _logger(logger)
	, _cmdCloseAppWindow(cmdClose)
	, _renderScheme(texman)
	, _worldView(texman, _renderScheme)
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

	_con = std::make_shared<UI::Console>(manager, texman);
	_con->SetBuffer(&_logger);
	_con->eventOnSendCommand = std::bind(&Desktop::OnCommand, this, _1);
	_con->eventOnRequestCompleteCommand = std::bind(&Desktop::OnCompleteCommand, this, _1, _2, _3);
	_con->SetVisible(false);
	_con->SetTopMost(true);
	SpriteColor colors[] = {0xffffffff, 0xffff7fff};
	_con->SetColors(colors, sizeof(colors) / sizeof(colors[0]));
	_con->SetHistory(&_history);
	AddFront(_con);

	if( _conf.d_graph.Get() )
	{
		float xx = 200;
		float yy = 3;
		float hh = 50;
		for( size_t i = 0; i < CounterBase::GetMarkerCountStatic(); ++i )
		{
			auto os = std::make_shared<Oscilloscope>();
//			os->Move(xx, yy);
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
	_pauseButton->SetTopMost(true);
	_pauseButton->SetBackground("ui/pause");
	_pauseButton->AlignToBackground(_texman);
	_pauseButton->eventClick = [=]()
	{
		assert(_game && _game->CanPause());
		_game->ShowPauseMenu();
	};
	AddFront(_pauseButton);

	_backButton = std::make_shared<UI::Button>();
	_backButton->SetTopMost(true);
	_backButton->SetBackground("ui/back");
	_backButton->AlignToBackground(_texman);
	_backButton->eventClick = [=]()
	{
		if (_game && _game.get() == _navStack->GetNavFront() && _game->CanNavigateBack())
			_game->NavigateBack();
		else if (CanNavigateBack())
			NavigateBack();
	};
	AddFront(_backButton);

	_navStack = std::make_shared<NavStack>(manager);
	_navStack->SetSpacing(_conf.ui_nav_spacing.GetFloat());
	AddFront(_navStack);

	ShowMainMenu();
	OnGameContextAdded();

	_conf.d_showfps.eventChange = std::bind(&Desktop::OnChangeShowFps, this);
	OnChangeShowFps();
}

Desktop::~Desktop()
{
	_conf.d_showfps.eventChange = nullptr;
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

vec2d Desktop::GetListenerPos() const
{
	return _game ? _game->GetListenerPos() : vec2d{};
}

void Desktop::OnCloseChild(std::shared_ptr<UI::Window> child)
{
	_navStack->PopNavStack(child.get());
}

void Desktop::OnNewCampaign()
{
	auto dlg = std::make_shared<NewCampaignDlg>(_fs, _lang);
	dlg->eventCampaignSelected = [this, weakSender = std::weak_ptr<NewCampaignDlg>(dlg)](std::string_view name)
	{
		if (auto sender = weakSender.lock())
		{
			OnCloseChild(sender);
			if (!name.empty())
			{
				try
				{
//					script_exec_file(_globL.get(), _fs, ("campaign/" + name + ".lua").c_str());
					throw std::logic_error("not implemented");
				}
				catch (const std::exception & e)
				{
					_logger.WriteLine(1, e.what());
					ShowConsole(true);
				}
			}
		}
	};
	_navStack->PushNavStack(dlg);
}

void Desktop::OnSinglePlayer()
{
	if (_navStack->IsOnStack<NewGameDlg>() || _navStack->IsOnStack<SinglePlayer>())
		return;

	if (_dmCampaign.tiers.GetSize() > 0)
	{
		auto dlg = std::make_shared<SinglePlayer>(_worldView, _fs, _appConfig, _conf, _dmCampaign, _mapCollection, _lang);
		dlg->eventSelectMap = [this, weakSender = std::weak_ptr<SinglePlayer>(dlg)](int index)
		{
			if (auto sender = weakSender.lock())
			{
				_conf.sp_map.SetInt(index);
				try
				{
					// previous game/editor context may be held by UI, release it first to prevent memory spike
					_navStack->Trim();

					int currentTier = GetCurrentTier(_conf, _dmCampaign);
					int currentMap = GetCurrentMap(_conf, _dmCampaign);
					_appController.StartDMCampaignMap(GetAppState(), _mapCollection, _appConfig, _dmCampaign, currentTier, currentMap);
				}
				catch (const std::exception & e)
				{
					_logger.Printf(1, "Could not start new game - %s", e.what());
				}
			}
		};
		_navStack->PushNavStack(dlg);
	}
}

void Desktop::OnSplitScreen()
{
	if (_navStack->IsOnStack<NewGameDlg>() || _navStack->IsOnStack<SinglePlayer>())
		return;

	auto dlg = std::make_shared<NewGameDlg>(_texman, _fs, _conf, _logger, _lang);
	dlg->eventClose = [this, weakSender = std::weak_ptr<NewGameDlg>(dlg)](int result)
	{
		if (auto sender = weakSender.lock())
		{
			OnCloseChild(sender);
			if (UI::Dialog::_resultOK == result)
			{
				try
				{
//					_appController.NewGameDM(GetAppState(), _conf.cl_map.Get(), GetDMSettingsFromConfig(_conf));
				}
				catch (const std::exception & e)
				{
					_logger.Printf(1, "Could not start new game - %s", e.what());
				}
			}
		}
	};
	_navStack->PushNavStack(dlg);
}

void Desktop::OnOpenMap()
{
	auto selectMapDlg = std::make_shared<SelectMapDlg>(_fs, _worldView, _conf, _lang, _mapCollection);
	selectMapDlg->eventMapSelected = [this, weakSender = std::weak_ptr<SelectMapDlg>(selectMapDlg)](unsigned int mapIndex)
	{
		if (auto sender = weakSender.lock())
		{
			// previous game/editor context may be held by UI, release it first to prevent memory spike
			_navStack->Trim();
			_appController.StartNewMapEditor(GetAppState(), _mapCollection, _conf.editor.width.GetInt(), _conf.editor.height.GetInt(),
				mapIndex != -1 ? _mapCollection.GetMapName(mapIndex) : "");
		}
	};

	_navStack->PushNavStack(selectMapDlg);
}

void Desktop::OnExportMap()
{
	if (GetAppState().GetGameContext())
	{
		GetFileNameDlg::Params param;
		param.title = _lang.get_file_name_save_map.Get();
		param.folder = _fs.GetFileSystem(DIR_MAPS, true);
		param.extension = "tzod";

		if (!param.folder)
		{
			ShowConsole(true);
			_logger.Printf(1, "ERROR: Could not open directory '%s'", DIR_MAPS);
			return;
		}

		auto fileDlg = std::make_shared<GetFileNameDlg>(param, _lang);
		fileDlg->eventClose = [this, weakSender=std::weak_ptr<GetFileNameDlg>(fileDlg)](int result)
		{
			if (auto sender = weakSender.lock())
			{
				OnCloseChild(sender);
				auto gameContext = GetAppState().GetGameContext();
				if (UI::Dialog::_resultOK == result && gameContext)
				{
					auto fileName = std::string(DIR_MAPS) + "/" + sender->GetFileName();
					gameContext->GetWorld().Export(*_fs.Open(fileName, FS::ModeWrite)->QueryStream());
					_logger.Printf(0, "map exported: '%s'", fileName.c_str());
				}
			}
		};
		_navStack->PushNavStack(fileDlg);
	}
}

void Desktop::OnSettingsMain()
{
	if (_navStack->IsOnStack<MainSettingsDlg>())
		return;

	MainSettingsCommands commands;
	commands.player = std::bind(&Desktop::OnPlayerSettings, this);
	commands.controls = std::bind(&Desktop::OnControlsSettings, this);
	commands.advanced = std::bind(&Desktop::OnAdvancedSettings, this);
	_navStack->PushNavStack(std::make_shared<MainSettingsDlg>(_lang, std::move(commands)), 1);
}

void Desktop::OnPlayerSettings()
{
	if (_navStack->IsOnStack<PlayerSettings>())
		return;

	_navStack->PushNavStack(std::make_shared<PlayerSettings>(_appConfig, _lang), 1);
}

void Desktop::OnControlsSettings()
{
	if (_navStack->IsOnStack<ControlsSettings>())
		return;

	_navStack->PushNavStack(std::make_shared<ControlsSettings>(_conf, _lang), 1);
}

void Desktop::OnAdvancedSettings()
{
	if (_navStack->IsOnStack<AdvancedSettings>())
		return;

	_navStack->PushNavStack(std::make_shared<AdvancedSettings>(_conf, _lang), 1);
}

void Desktop::OnMapSettings()
{
	if (auto gameContext = GetAppState().GetGameContext())
	{
		auto dlg = std::make_shared<MapSettingsDlg>(gameContext->GetWorld(), _lang);
		dlg->eventClose = [this, weakSender = std::weak_ptr<MapSettingsDlg>(dlg)](int result)
		{
			OnCloseChild(weakSender.lock());
		};
		_navStack->PushNavStack(dlg);
	}
}

void Desktop::ShowMainMenu()
{
	if (_navStack->IsOnTop<MainMenuDlg>())
		return;
	assert(!CanNavigateBack());

	MainMenuCommands commands;
	commands.singlePlayer = std::bind(&Desktop::OnSinglePlayer, this);
	commands.splitScreen = std::bind(&Desktop::OnSplitScreen, this);
	commands.openMap = std::bind(&Desktop::OnOpenMap, this);
	commands.exportMap = std::bind(&Desktop::OnExportMap, this);
	commands.gameSettings = std::bind(&Desktop::OnSettingsMain, this);
	if (_cmdCloseAppWindow)
	{
		commands.quitGame = [=] { _cmdCloseAppWindow->RequestClose(); };
	}
	_navStack->PushNavStack(std::make_shared<MainMenuDlg>(_lang, std::move(commands)), 1);
}

std::shared_ptr<const UI::Window> Desktop::GetFocus(const std::shared_ptr<const UI::Window>& owner) const
{
	if (_con->GetVisible() && !_consoleKeyPressed)
	{
		return _con;
	}
	else if (_navStack->GetNavFront())
	{
		return _navStack;
	}
	return nullptr;
}

const UI::Window* Desktop::GetFocus() const
{
	if (_con->GetVisible() && !_consoleKeyPressed)
	{
		return _con.get();
	}
	else if (_navStack->GetNavFront())
	{
		return _navStack.get();
	}
	return nullptr;
}

void Desktop::NavigateBack()
{
	if (GetFocus() == _con.get())
	{
		_con->SetVisible(false);
	}
	else
	{
		if (_navStack->GetNavFront() == _game.get())
		{
			GetAppState().PopGameContext();
			assert(!_game);
		}
		else if (_navStack->IsOnTop<EditorMain>())
		{
			_appController.SaveAndExitEditor(GetAppState(), _mapCollection);
		}
		else
		{
			_navStack->PopNavStack();
		}
	}
}

bool Desktop::CanNavigateBack() const
{
	if (GetFocus() == _con.get())
		return true;

	// cannot navigate back past the game until it's over
	if (_game && _game.get() == _navStack->GetNavFront() && !_game->GetGameOver())
		return false;

	// Cannot navigate back past the main menu
	return _navStack->GetNavFront() && !_navStack->IsOnTop<MainMenuDlg>();
}

bool Desktop::OnKeyPressed(const Plat::Input &input, const UI::InputContext &ic, Plat::Key key)
{
	switch( key )
	{
	case Plat::Key::GraveAccent: // '~'
		_con->SetVisible(!_con->GetVisible());
		_consoleKeyPressed = true;
		break;

	case Plat::Key::F2:
		OnSinglePlayer();
		break;

	case Plat::Key::F12:
		OnSettingsMain();
		break;

	case Plat::Key::F8:
		OnMapSettings();
		break;

	case Plat::Key::F10:
		if (_conf.d_artistmode.Get())
		{
			ImageCache imageCache;
			_texman.LoadPackage(_fs, imageCache, ParseDirectory(_fs, DIR_SPRITES));
		}
		break;

	default:
		return false;
	}

	return true;
}

void Desktop::OnKeyReleased(const UI::InputContext &ic, Plat::Key key)
{
	if (Plat::Key::GraveAccent == key)
	{
		_consoleKeyPressed = false;
	}
}

bool Desktop::CanNavigate(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate) const
{
	return UI::Navigate::Back == navigate && CanNavigateBack();
}

void Desktop::OnNavigate(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate, UI::NavigationPhase phase)
{
	if (UI::NavigationPhase::Completed == phase && UI::Navigate::Back == navigate)
	{
		NavigateBack();
	}
}

UI::WindowLayout Desktop::GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	auto size = lc.GetPixelSize();
	auto scale = lc.GetScaleCombined();
	if (_background.get() == &child)
	{
		float transition = (1 - std::cos(PI * _navStack->GetInterpolatedAttribute())) / 2;
		return UI::WindowLayout{ AlignCC(size * Lerp(1.5f, 1, transition), size), transition, true };
	}
	if (_navStack.get() == &child)
	{
		return UI::WindowLayout{ MakeRectWH(size), 1, true };
	}
	if (_con.get() == &child)
	{
		return UI::WindowLayout{ MakeRectRB(Vec2dFloor(vec2d{ 10, 0 } * scale), Vec2dFloor(size.x - 10 * scale, size.y / 2)), 1, true };
	}
	if (_fps.get() == &child)
	{
		return UI::WindowLayout{ AlignLB(_fps->GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc)), size.y), 1, true };
	}
	if (_tierTitle.get() == &child)
	{
		float opacity = 0;
		if (auto gameContext = dynamic_cast<GameContext*>(GetAppState().GetGameContext().get()))
			opacity = std::max(0.f, std::min(1.f, (5 - gameContext->GetWorld().GetTime()) / 3));
		return UI::WindowLayout{ MakeRectWH(Vec2dFloor(size / 2), vec2d{}), opacity, true };
	}
	if (_backButton.get() == &child)
	{
		bool canNavigateBack = UI::CanNavigateBack(texman, *this, lc, dc);
		return UI::WindowLayout{ MakeRectWH(child.GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc))), (float) canNavigateBack /*opacity*/, canNavigateBack /*enabled*/ };
	}
	if (_pauseButton.get() == &child)
	{
		bool canNavigateBack = UI::CanNavigateBack(texman, *this, lc, dc);
		bool canPause = !canNavigateBack && _game && _game->CanPause();
		return UI::WindowLayout{ MakeRectWH(child.GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc))), (float)canPause /*opacity*/, canPause /*enabled*/ };
	}
	assert(false);
	return {};
}

void Desktop::OnChangeShowFps()
{
	if (_conf.d_showfps.Get() && !_fps)
	{
		_fps = std::make_shared<FpsCounter>(GetTimeStepManager(), alignTextLB, GetAppState());
		AddFront(_fps);
	}
	else if (!_conf.d_showfps.Get() && _fps)
	{
		UnlinkChild(*_fps);
		_fps.reset();
	}
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

void Desktop::OnGameContextRemoving()
{
	if (dynamic_cast<GameContext*>(GetAppState().GetGameContext().get()))
	{
		while (_navStack->IsOnStack<GameLayout>())
			_navStack->PopNavStack();
		_game.reset();
	}
	else if (dynamic_cast<EditorContext*>(GetAppState().GetGameContext().get()))
	{
		while (_navStack->IsOnStack<EditorMain>())
			_navStack->PopNavStack();
	}
}

void Desktop::OnGameContextRemoved()
{
}

void Desktop::OnGameContextAdded()
{
	if (auto gameContext = std::dynamic_pointer_cast<GameContext>(GetAppState().GetGameContext()))
	{
		assert(!_game);

		CampaignControlCommands campaignControlCommands;
		if (dynamic_cast<GameContextCampaignDM*>(gameContext.get()))
		{
			campaignControlCommands.replayCurrent = [this]
			{
				_appController.StartDMCampaignMap(
					GetAppState(),
					_mapCollection,
					_appConfig,
					_dmCampaign,
					GetCurrentTier(_conf, _dmCampaign),
					GetCurrentMap(_conf, _dmCampaign));
			};
			campaignControlCommands.playNext = [this]
			{
				int tierIndex = GetCurrentTier(_conf, _dmCampaign);
				int nextMapIndex = (GetCurrentMap(_conf, _dmCampaign) + 1) % GetCurrentTierMapCount(_conf, _dmCampaign);
				if (nextMapIndex == 0 && IsTierComplete(_appConfig, _dmCampaign, tierIndex))
					tierIndex++;
				_conf.sp_map.SetInt(nextMapIndex);
				_conf.sp_tier.SetInt(tierIndex);
				_appController.StartDMCampaignMap(GetAppState(), _mapCollection, _appConfig, _dmCampaign, tierIndex, nextMapIndex);
			};
		}
		else
		{
			campaignControlCommands.replayCurrent = [this]
			{
				GetAppState().PopGameContext();
				_appController.PlayCurrentMap(GetAppState(), _mapCollection);
			};
		}
		campaignControlCommands.quitCurrent = [this]
		{
			GetAppState().PopGameContext();
		};
		campaignControlCommands.systemSettings = std::bind(&Desktop::OnSettingsMain, this);

		_game = std::make_shared<GameLayout>(
			GetTimeStepManager(),
			gameContext,
			_worldView,
			gameContext->GetWorldController(),
			_conf,
			_lang,
			_logger,
			std::move(campaignControlCommands));
		_navStack->PushNavStack(_game);

		int currentTier = GetCurrentTier(_conf, _dmCampaign);
		DMCampaignTier tierDesc(&_dmCampaign.tiers.GetTable(currentTier));
		_tierTitle->SetText(ConfBind(tierDesc.title));
	}
	else if (auto editorContext = std::dynamic_pointer_cast<EditorContext>(GetAppState().GetGameContext()))
	{
		_navStack->Trim();

		auto editor = std::make_shared<EditorMain>(
			GetTimeStepManager(),
			_texman,
			std::move(editorContext),
			_worldView,
			_conf.editor,
			_lang,
			EditorCommands{ [this] { _appController.PlayCurrentMap(GetAppState(), _mapCollection); } },
			_logger);

		_navStack->PushNavStack(editor);
	}
}
