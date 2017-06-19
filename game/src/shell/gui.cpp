#include "gui.h"
#include "ConfigBinding.h"
#include "MapList.h"
#include "inc/shell/Config.h"

#include <gc/Player.h>
#include <gc/VehicleClasses.h>
#include <loc/Language.h>
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/ListBox.h>
#include <ui/ListSelectionBinding.h>
#include <ui/MultiColumnListItem.h>
#include <ui/Text.h>
#include <ui/Edit.h>
#include <ui/EditableText.h>
#include <ui/Combo.h>
#include <ui/DataSourceAdapters.h>
#include <ui/Keys.h>
#include <ui/ScrollView.h>
#include <video/TextureManager.h>

#include <algorithm>
#include <sstream>


#define MAX_GAMESPEED   200
#define MIN_GAMESPEED   20

#define MAX_TIMELIMIT   1000
#define MAX_FRAGLIMIT   10000

NewGameDlg::NewGameDlg(TextureManager &texman, FS::FileSystem &fs, ShellConfig &conf, UI::ConsoleBuffer &logger, LangCache &lang)
  : _texman(texman)
  , _conf(conf)
  , _lang(lang)
{
	Resize(770, 550);

	_newPlayer = false;

	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 20;


	//
	// map list
	//

	auto text = std::make_shared<UI::Text>();
	text->Move(16, 16);
	text->SetText(ConfBind(_lang.choose_map));
	AddFront(text);

	auto mapListItemTemplate = std::make_shared<UI::MultiColumnListItem>();
	mapListItemTemplate->EnsureColumn(0, 4); // name
	mapListItemTemplate->EnsureColumn(1, 384); // size
	mapListItemTemplate->EnsureColumn(2, 448); // theme

	_maps = std::make_shared<MapList>(fs, logger);
	_maps->GetList()->SetCurSel(_maps->GetData()->FindItem(conf.cl_map.Get()), false);
	_maps->GetList()->SetItemTemplate(mapListItemTemplate);
//	_maps->SetScrollPos(_maps->GetCurSel() - (_maps->GetNumLinesVisible() - 1) * 0.5f);
	_maps->Move(x1, 32);
	_maps->Resize(x2 - x1, 192);

	AddFront(_maps);
	SetFocus(_maps);


	//
	// settings
	//

	{
		float y =  16;

		_nightMode = std::make_shared<UI::CheckBox>();
		_nightMode->Move(x3, y);
		_nightMode->SetText(ConfBind(_lang.night_mode));
		_nightMode->SetCheck(conf.cl_nightmode.Get());
		AddFront(_nightMode);

		text = std::make_shared<UI::Text>();
		text->Move(x3, y += 30);
		text->SetText(ConfBind(_lang.game_speed));
		AddFront(text);

		_gameSpeed = std::make_shared<UI::Edit>();
		_gameSpeed->Move(x3 + 20, y += 15);
		_gameSpeed->SetWidth(80);
		_gameSpeed->GetEditable()->SetInt(conf.cl_speed.GetInt());
		AddFront(_gameSpeed);

		text = std::make_shared<UI::Text>();
		text->Move(x3, y += 30);
		text->SetText(ConfBind(_lang.frag_limit));
		AddFront(text);

		_fragLimit = std::make_shared<UI::Edit>();
		_fragLimit->Move(x3 + 20, y += 15);
		_fragLimit->SetWidth(80);
		_fragLimit->GetEditable()->SetInt(conf.cl_fraglimit.GetInt());
		AddFront(_fragLimit);

		text = std::make_shared<UI::Text>();
		text->Move(x3, y += 30);
		text->SetText(ConfBind(_lang.time_limit));
		AddFront(text);

		_timeLimit = std::make_shared<UI::Edit>();
		_timeLimit->Move(x3 + 20, y += 15);
		_timeLimit->SetWidth(80);
		_timeLimit->GetEditable()->SetInt(conf.cl_timelimit.GetInt());
		AddFront(_timeLimit);

		text = std::make_shared<UI::Text>();
		text->Move(x3 + 30, y += 30);
		text->SetText(ConfBind(_lang.zero_no_limits));
		AddFront(text);
	}



	//
	// player list
	//

	text = std::make_shared<UI::Text>();
	text->Move(16, 240);
	text->SetText(ConfBind(_lang.human_player_list));
	AddFront(text);

	auto playerListItemTemplate = std::make_shared<UI::MultiColumnListItem>();
	playerListItemTemplate->EnsureColumn(0, 4); // name
	playerListItemTemplate->EnsureColumn(1, 192); // skin
	playerListItemTemplate->EnsureColumn(2, 256); // class
	playerListItemTemplate->EnsureColumn(3, 320); // team

	_players = std::make_shared<DefaultListBox>();
	_players->Move(x1, 256);
	_players->Resize(x2-x1, 96);
	_players->GetList()->SetItemTemplate(playerListItemTemplate);
	AddFront(_players);


	text = std::make_shared<UI::Text>();
	text->Move(16, 368);
	text->SetText(ConfBind(_lang.AI_player_list));
	AddFront(text);

	_bots = std::make_shared<DefaultListBox>();
	_bots->Move(x1, 384);
	_bots->Resize(x2-x1, 96);
	_bots->GetList()->SetItemTemplate(playerListItemTemplate);
	AddFront(_bots);


	//
	// buttons
	//

	{
		std::shared_ptr<UI::Button> btn;

		btn = std::make_shared<UI::Button>();
		btn->SetText(ConfBind(_lang.human_player_add));
		btn->Move(x3, 256);
		btn->eventClick = std::bind(&NewGameDlg::OnAddPlayer, this);
		AddFront(btn);

		_removePlayer = std::make_shared<UI::Button>();
		_removePlayer->SetText(ConfBind(_lang.human_player_remove));
		_removePlayer->Move(x3, 286);
		_removePlayer->eventClick = std::bind(&NewGameDlg::OnRemovePlayer, this);
		_removePlayer->SetEnabled(std::make_shared<UI::HasSelection>(_players->GetList()));
		AddFront(_removePlayer);

		_changePlayer = std::make_shared<UI::Button>();
		_changePlayer->SetText(ConfBind(_lang.human_player_modify));
		_changePlayer->Move(x3, 316);
		_changePlayer->eventClick = std::bind(&NewGameDlg::OnEditPlayer, this);
		_changePlayer->SetEnabled(std::make_shared<UI::HasSelection>(_players->GetList()));
		AddFront(_changePlayer);

		btn = std::make_shared<UI::Button>();
		btn->SetText(ConfBind(_lang.AI_player_add));
		btn->Move(x3, 384);
		btn->eventClick = std::bind(&NewGameDlg::OnAddBot, this);
		AddFront(btn);

		_removeBot = std::make_shared<UI::Button>();
		_removeBot->SetText(ConfBind(_lang.AI_player_remove));
		_removeBot->Move(x3, 414);
		_removeBot->eventClick = std::bind(&NewGameDlg::OnRemoveBot, this);
		_removeBot->SetEnabled(std::make_shared<UI::HasSelection>(_bots->GetList()));
		AddFront(_removeBot);

		_changeBot = std::make_shared<UI::Button>();
		_changeBot->SetText(ConfBind(_lang.AI_player_modify));
		_changeBot->Move(x3, 444);
		_changeBot->eventClick = std::bind(&NewGameDlg::OnEditBot, this);
		_changeBot->SetEnabled(std::make_shared<UI::HasSelection>(_bots->GetList()));
		AddFront(_changeBot);


		btn = std::make_shared<UI::Button>();
		btn->SetText(ConfBind(_lang.dm_ok));
		btn->Move(544, 510);
		btn->eventClick = std::bind(&NewGameDlg::OnOK, this);
		AddFront(btn);
	}

	// call this after creation of buttons
	RefreshPlayersList();
	RefreshBotsList();
}

NewGameDlg::~NewGameDlg()
{
}

void NewGameDlg::RefreshPlayersList()
{
	int selected = _players->GetList()->GetCurSel();
	_players->GetData()->DeleteAllItems();

	for( size_t i = 0; i < _conf.dm_players.GetSize(); ++i )
	{
		ConfPlayerLocal p(&_conf.dm_players.GetAt(i).AsTable());

		int index = _players->GetData()->AddItem(p.nick.Get());
		_players->GetData()->SetItemText(index, 1, p.skin.Get());
		_players->GetData()->SetItemText(index, 2, p.platform_class.Get());

		std::ostringstream s;
		if( int team = p.team.GetInt() )
			s << team;
		else
			s << _lang.team_none.Get();
		_players->GetData()->SetItemText(index, 3, s.str());
	}

	_players->GetList()->SetCurSel(std::min(selected, _players->GetData()->GetItemCount()-1));
}

void NewGameDlg::RefreshBotsList()
{
	int selected = _bots->GetList()->GetCurSel();
	_bots->GetData()->DeleteAllItems();

	for( size_t i = 0; i < _conf.dm_bots.GetSize(); ++i )
	{
		ConfPlayerAI p(&_conf.dm_bots.GetAt(i).AsTable());

		int index = _bots->GetData()->AddItem(p.nick.Get());
		_bots->GetData()->SetItemText(index, 1, p.skin.Get());
		_bots->GetData()->SetItemText(index, 2, p.platform_class.Get());

		std::ostringstream s;
		if( int team = p.team.GetInt() )
			s << team;
		else
			s << _lang.team_none.Get();
		_bots->GetData()->SetItemText(index, 3, s.str());
	}

	_bots->GetList()->SetCurSel(std::min(selected, _bots->GetData()->GetItemCount() - 1));
}

void NewGameDlg::OnAddPlayer()
{
	std::vector<std::string> skinNames;
	_texman.GetTextureNames(skinNames, "skin/");

	ConfVarTable &p = _conf.dm_players.PushBack(ConfVar::typeTable).AsTable();
	p.SetStr("skin", skinNames[rand() % skinNames.size()]);

	_newPlayer = true;
	auto dlg = std::make_shared<EditPlayerDlg>(_texman, p, _conf, _lang);
	dlg->eventClose = std::bind(&NewGameDlg::OnAddPlayerClose, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
	SetFocus(dlg);
}

void NewGameDlg::OnAddPlayerClose(std::shared_ptr<UI::Dialog> sender, int result)
{
	if( _resultOK == result )
	{
		RefreshPlayersList();
	}
	else if( _newPlayer )
	{
		_conf.dm_players.PopBack();
	}
	_newPlayer = false;
	UnlinkChild(*sender);
}

void NewGameDlg::OnRemovePlayer()
{
	assert(-1 != _players->GetList()->GetCurSel());
	_conf.dm_players.RemoveAt(_players->GetList()->GetCurSel());
	RefreshPlayersList();
}

void NewGameDlg::OnEditPlayer()
{
	int index = _players->GetList()->GetCurSel();
	assert(-1 != index);

	auto dlg = std::make_shared<EditPlayerDlg>(_texman, _conf.dm_players.GetAt(index).AsTable(), _conf, _lang);
	dlg->eventClose = std::bind(&NewGameDlg::OnEditPlayerClose, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
	SetFocus(dlg);
}

void NewGameDlg::OnEditPlayerClose(std::shared_ptr<UI::Dialog> sender, int result)
{
	if( _resultOK == result )
	{
		RefreshPlayersList();
	}
	UnlinkChild(*sender);
}

void NewGameDlg::OnAddBot()
{
	std::vector<std::string> skinNames;
	_texman.GetTextureNames(skinNames, "skin/");

	ConfVarTable &p = _conf.dm_bots.PushBack(ConfVar::typeTable).AsTable();
	p.SetStr("skin", skinNames[rand() % skinNames.size()]);

	_newPlayer = true;
	auto dlg = std::make_shared<EditBotDlg>(_texman, p, _lang);
	dlg->eventClose = std::bind(&NewGameDlg::OnAddBotClose, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
	SetFocus(dlg);
}

void NewGameDlg::OnAddBotClose(std::shared_ptr<UI::Dialog> sender, int result)
{
	if( _resultOK == result )
	{
		RefreshBotsList();
	}
	else if( _newPlayer )
	{
		_conf.dm_bots.PopBack();
	}
	_newPlayer = false;
	UnlinkChild(*sender);
}

void NewGameDlg::OnRemoveBot()
{
	assert(-1 != _bots->GetList()->GetCurSel());
	_conf.dm_bots.RemoveAt(_bots->GetList()->GetCurSel());
	RefreshBotsList();
}

void NewGameDlg::OnEditBot()
{
	int index = _bots->GetList()->GetCurSel();
	assert(-1 != index);

	auto dlg = std::make_shared<EditBotDlg>(_texman, _conf.dm_bots.GetAt(index).AsTable(), _lang);
	dlg->eventClose = std::bind(&NewGameDlg::OnEditBotClose, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
	SetFocus(dlg);
}

void NewGameDlg::OnEditBotClose(std::shared_ptr<UI::Dialog> sender, int result)
{
	if( _resultOK == result )
	{
		RefreshBotsList();
	}
	UnlinkChild(*sender);
}

void NewGameDlg::OnOK()
{
	std::string fn;
	int index = _maps->GetList()->GetCurSel();
	if( -1 != index )
	{
		fn = _maps->GetData()->GetItemText(index, 0);
	}
	else
	{
		return;
	}

	_conf.cl_speed.SetInt(std::max(MIN_GAMESPEED, std::min(MAX_GAMESPEED, _gameSpeed->GetEditable()->GetInt())));
	_conf.cl_fraglimit.SetInt(std::max(0, std::min(MAX_FRAGLIMIT, _fragLimit->GetEditable()->GetInt())));
	_conf.cl_timelimit.SetInt(std::max(0, std::min(MAX_TIMELIMIT, _timeLimit->GetEditable()->GetInt())));
	_conf.cl_nightmode.Set(_nightMode->GetCheck());

	_conf.sv_speed.SetInt(_conf.cl_speed.GetInt());
	_conf.sv_fraglimit.SetInt(_conf.cl_fraglimit.GetInt());
	_conf.sv_timelimit.SetInt(_conf.cl_timelimit.GetInt());
	_conf.sv_nightmode.Set(_conf.cl_nightmode.Get());

	_conf.cl_map.Set(fn);

	Close(_resultOK);
}

bool NewGameDlg::OnKeyPressed(UI::InputContext &ic, UI::Key key)
{
	switch(key)
	{
	case UI::Key::Enter:
	case UI::Key::NumEnter:
		if( GetFocus() == _players && -1 != _players->GetList()->GetCurSel() )
			OnEditPlayer();
		else
			OnOK();
		break;
	case UI::Key::Insert:
		OnAddPlayer();
		break;
	default:
		return Dialog::OnKeyPressed(ic, key);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

EditPlayerDlg::EditPlayerDlg(TextureManager &texman, ConfVarTable &info, ShellConfig &conf, LangCache &lang)
  : _info(&info)
{
	Resize(384, 220);

	// Title
	auto text = std::make_shared<UI::Text>();
	text->Move(GetWidth() / 2, 16);
	text->SetText(ConfBind(lang.player_settings));
	text->SetAlign(alignTextCT);
	text->SetFont("font_default");
	AddFront(text);

	float x1 = 30;
	float x2 = x1 + 56;
	float y = 56;

	_skinPreview = std::make_shared<Rectangle>();
	_skinPreview->Move(300, y);
	AddFront(_skinPreview);


	//
	// player name field
	//

	text = std::make_shared<UI::Text>();
	text->Move(x1, y);
	text->SetText(ConfBind(lang.player_nick));
	AddFront(text);

	_name = std::make_shared<UI::Edit>();
	_name->Move(x2, y -= 1);
	_name->SetWidth(200);
	_name->GetEditable()->SetText(_info.nick.Get() );
	AddFront(_name);
	SetFocus(_name);


	//
	// skins combo
	//
	text = std::make_shared<UI::Text>();
	text->Move(x1, y += 24);
	text->SetText(ConfBind(lang.player_skin));
	AddFront(text);

	_skins = std::make_shared<DefaultComboBox>();
	_skins->Move(x2, y -= 1);
	_skins->SetWidth(200);
	_skins->eventChangeCurSel = std::bind(&EditPlayerDlg::OnChangeSkin, this, std::placeholders::_1);
	AddFront(_skins);
	std::vector<std::string> names;
	texman.GetTextureNames(names, "skin/");
	for( size_t i = 0; i < names.size(); ++i )
	{
		int index = _skins->GetData()->AddItem(names[i]);
		if( names[i] == _info.skin.Get() )
		{
			_skins->SetCurSel(index);
		}
	}
	if( -1 == _skins->GetCurSel() )
	{
		_skins->SetCurSel(0);
	}


	//
	// create and fill the classes list
	//

	text = std::make_shared<UI::Text>();
	text->Move(x1, y += 24);
	text->SetText(ConfBind(lang.player_class));
	AddFront(text);

	_classes = std::make_shared<DefaultComboBox>();
	_classes->Move(x2, y -= 1);
	_classes->SetWidth(200);
	AddFront(_classes);

	std::pair<std::string, std::string> val;
	for( unsigned int i = 0; GetVehicleClassName(i); ++i )
	{
		val.first = GetVehicleClassName(i);
		val.second = GetVehicleClassName(i);
		_classNames.push_back(val);

		if( std::string::npos == val.first.find('/') )
		{
			int index = _classes->GetData()->AddItem(val.first);
			if( val.first == _info.platform_class.Get() )
			{
				_classes->SetCurSel(index);
			}
		}
	}
	if( -1 == _classes->GetCurSel() )
		_classes->SetCurSel(0);

	text = std::make_shared<UI::Text>();
	text->Move(x1, y += 24);
	text->SetText(ConfBind(lang.player_team));
	AddFront(text);

	_teams = std::make_shared<DefaultComboBox>();
	_teams->Move(x2, y -= 1);
	_teams->SetWidth(200);
	AddFront(_teams);
	for( int i = 0; i < MAX_TEAMS; ++i )
	{
		std::ostringstream s;
		if( i )
			s << i;
		else
			s << lang.team_none.Get();
		int index = _teams->GetData()->AddItem(s.str());
		if( i == _info.team.GetInt() )
		{
			_teams->SetCurSel(index);
		}
	}
	if( -1 == _teams->GetCurSel() )
	{
		_teams->SetCurSel(0);
	}



	//
	// player profile combo
	//

	text = std::make_shared<UI::Text>();
	text->Move(x1, y += 24);
	text->SetText(ConfBind(lang.player_profile));
	AddFront(text);

	_profiles = std::make_shared<DefaultComboBox>();
	_profiles->Move(x2, y -= 1);
	_profiles->SetWidth(200);
	AddFront(_profiles);
	std::vector<std::string> profiles = conf.dm_profiles.GetKeys();

	for( size_t i = 0; i < profiles.size(); ++i )
	{
		int index = _profiles->GetData()->AddItem(profiles[i]);
		if( profiles[i] == _info.profile.Get() )
		{
			_profiles->SetCurSel(index);
		}
	}
	if( -1 == _profiles->GetCurSel() )
	{
		_profiles->SetCurSel(0);
	}


	//
	// create buttons
	//

	auto btn = std::make_shared<UI::Button>();
	btn->SetText(ConfBind(lang.common_ok));
	btn->Move(176, 190);
	btn->eventClick = std::bind(&Dialog::Close, this, _resultOK);
	AddFront(btn);

	btn = std::make_shared<UI::Button>();
	btn->SetText(ConfBind(lang.common_cancel));
	btn->Move(280, 190);
	btn->eventClick = std::bind(&Dialog::Close, this, _resultCancel);
	AddFront(btn);
}

void EditPlayerDlg::OnChangeSkin(int index)
{
	if( -1 != index )
	{
		_skinPreview->SetTexture("skin/" + _skins->GetData()->GetItemText(index, 0));
	}
}

bool EditPlayerDlg::OnClose(int result)
{
	_info.nick.Set(_name->GetEditable()->GetText());
	_info.skin.Set(_skins->GetData()->GetItemText(_skins->GetCurSel(), 0));
	_info.platform_class.Set(_classes->GetData()->GetItemText(_classes->GetCurSel(), 0));
	_info.team.SetInt(_teams->GetCurSel());
	_info.profile.Set(_profiles->GetData()->GetItemText(_profiles->GetCurSel(), 0));

	return true;
}

FRECT EditPlayerDlg::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const Window &child) const
{
	if (_skinPreview.get() == &child)
	{
		vec2d size = { _skinPreview->GetTextureWidth(texman), _skinPreview->GetTextureHeight(texman) };
		return UI::CanvasLayout(_skinPreview->GetOffset(), size, lc.GetScale());
	}
	return UI::Dialog::GetChildRect(texman, lc, dc, child);
}

///////////////////////////////////////////////////////////////////////////////

static const char* s_levels[16] = {
	"bot_difficulty_0",
	"bot_difficulty_1",
	"bot_difficulty_2",
	"bot_difficulty_3",
	"bot_difficulty_4",
};

EditBotDlg::EditBotDlg(TextureManager &texman, ConfVarTable &info, LangCache &lang)
  : _info(&info)
{
	Resize(384, 220);

	// Title
	auto text = std::make_shared<UI::Text>();
	text->Move(GetWidth() / 2, 16);
	text->SetText(ConfBind(lang.bot_settings));
	text->SetAlign(alignTextCT);
	text->SetFont("font_default");
	AddFront(text);


	float x1 = 30;
	float x2 = x1 + 56;
	float y = 56;

	_skinPreview = std::make_shared<Rectangle>();
	_skinPreview->Move(300, y);
	AddFront(_skinPreview);


	//
	// player name field
	//

	text = std::make_shared<UI::Text>();
	text->Move(x1, y);
	text->SetText(ConfBind(lang.player_nick));
	AddFront(text);

	_name = std::make_shared<UI::Edit>();
	_name->Move(x2, y -= 1);
	_name->SetWidth(200);
	_name->GetEditable()->SetText(_info.nick.Get().empty() ? "player" : _info.nick.Get());
	AddFront(_name);
	SetFocus(_name);


	//
	// skins combo
	//
	text = std::make_shared<UI::Text>();
	text->Move(x1, y += 24);
	text->SetText(ConfBind(lang.player_skin));
	AddFront(text);

	_skins = std::make_shared<DefaultComboBox>();
	_skins->Move(x2, y -= 1);
	_skins->SetWidth(200);
	_skins->eventChangeCurSel = std::bind(&EditBotDlg::OnChangeSkin, this, std::placeholders::_1);
	AddFront(_skins);
	std::vector<std::string> names;
	texman.GetTextureNames(names, "skin/");
	for( size_t i = 0; i < names.size(); ++i )
	{
		int index = _skins->GetData()->AddItem(names[i]);
		if( names[i] == _info.skin.Get() )
		{
			_skins->SetCurSel(index);
		}
	}
	if( -1 == _skins->GetCurSel() )
	{
		_skins->SetCurSel(0);
	}


	//
	// create and fill the classes list
	//

	text = std::make_shared<UI::Text>();
	text->Move(x1, y += 24);
	text->SetText(ConfBind(lang.player_class));
	AddFront(text);

	_classes = std::make_shared<DefaultComboBox>();
	_classes->Move(x2, y -= 1);
	_classes->SetWidth(200);
	AddFront(_classes);

	std::pair<std::string, std::string> val;
	for( unsigned int i = 0; GetVehicleClassName(i); ++i )
	{
		val.first = GetVehicleClassName(i);
		val.second = GetVehicleClassName(i);
		_classNames.push_back(val);

		int index = _classes->GetData()->AddItem(val.first);
		if( val.first == _info.platform_class.Get() )
		{
			_classes->SetCurSel(index);
		}
	}
	if( -1 == _classes->GetCurSel() )
		_classes->SetCurSel(0);


	text = std::make_shared<UI::Text>();
	text->Move(x1, y += 24);
	text->SetText(ConfBind(lang.player_team));
	AddFront(text);

	_teams = std::make_shared<DefaultComboBox>();
	_teams->Move(x2, y -= 1);
	_teams->SetWidth(200);
	AddFront(_teams);
	for( int i = 0; i < MAX_TEAMS; ++i )
	{
		std::ostringstream s;
		if( i )
			s << i;
		else
			s << lang.team_none.Get();
		int index = _teams->GetData()->AddItem(s.str());
		if( i == _info.team.GetInt() )
		{
			_teams->SetCurSel(index);
		}
	}
	if( -1 == _teams->GetCurSel() )
	{
		_teams->SetCurSel(0);
	}


	//
	// create and fill the difficulty list
	//

	text = std::make_shared<UI::Text>();
	text->Move(x1, y += 24);
	text->SetText(ConfBind(lang.bot_difficulty));
	AddFront(text);

	_levels = std::make_shared<DefaultComboBox>();
	_levels->Move(x2, y -= 1);
	_levels->SetWidth(200);
	AddFront(_levels);

	for( int i = 0; i < 5; ++i )
	{
		int index = _levels->GetData()->AddItem(lang->GetStr(s_levels[i]).Get());
		if( i == _info.level.GetInt() )
		{
			_levels->SetCurSel(index);
		}
	}
	if( -1 == _levels->GetCurSel() )
	{
		_levels->SetCurSel(2);
	}


	//
	// create buttons
	//

	auto btn = std::make_shared<UI::Button>();
	btn->SetText(ConfBind(lang.common_ok));
	btn->Move(176, 190);
	btn->eventClick = std::bind(&EditBotDlg::OnOK, this);
	AddFront(btn);

	btn = std::make_shared<UI::Button>();
	btn->SetText(ConfBind(lang.common_cancel));
	btn->Move(280, 190);
	btn->eventClick = std::bind(&EditBotDlg::OnCancel, this);
	AddFront(btn);
}

void EditBotDlg::OnOK()
{
	_info.nick.Set(_name->GetEditable()->GetText());
	_info.skin.Set(_skins->GetData()->GetItemText(_skins->GetCurSel(), 0) );
	_info.platform_class.Set(_classes->GetData()->GetItemText(_classes->GetCurSel(), 0));
	_info.team.SetInt(_teams->GetCurSel());
	_info.level.SetInt(_levels->GetCurSel());

	Close(_resultOK);
}

void EditBotDlg::OnCancel()
{
	Close(_resultCancel);
}

void EditBotDlg::OnChangeSkin(int index)
{
	if( -1 != index )
	{
		_skinPreview->SetTexture("skin/" + _skins->GetData()->GetItemText(index, 0));
	}
}

FRECT EditBotDlg::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const Window &child) const
{
	if (_skinPreview.get() == &child)
	{
		vec2d size = { _skinPreview->GetTextureWidth(texman), _skinPreview->GetTextureHeight(texman) };
		return UI::CanvasLayout(_skinPreview->GetOffset(), size, lc.GetScale());
	}
	return UI::Dialog::GetChildRect(texman, lc, dc, child);
}
