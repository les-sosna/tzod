#include "gui.h"
#include "MapList.h"
#include "inc/shell/Config.h"

#include <gc/Player.h>
#include <gc/VehicleClasses.h>
#include <loc/Language.h>
#include <ui/Button.h>
#include <ui/List.h>
#include <ui/Text.h>
#include <ui/Edit.h>
#include <ui/Combo.h>
#include <ui/DataSourceAdapters.h>
#include <ui/GuiManager.h>
#include <ui/Keys.h>
#include <video/TextureManager.h>

#include <algorithm>
#include <sstream>


#define MAX_GAMESPEED   200
#define MIN_GAMESPEED   20

#define MAX_TIMELIMIT   1000
#define MAX_FRAGLIMIT   10000


NewGameDlg::NewGameDlg(UI::LayoutManager &manager, TextureManager &texman, FS::FileSystem &fs, ConfCache &conf, UI::ConsoleBuffer &logger, LangCache &lang)
  : Dialog(manager, texman, 770, 550)
  , _texman(texman)
  , _conf(conf)
  , _lang(lang)
{
	_newPlayer = false;

	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 20;


	//
	// map list
	//

	UI::Text::Create(this, texman, 16, 16, _lang.choose_map.Get(), alignTextLT);

	_maps = std::make_shared<MapList>(manager, texman, fs, logger);
	_maps->Move(x1, 32);
	_maps->Resize(x2 - x1, 192);
	_maps->SetTabPos(0,   4); // name
	_maps->SetTabPos(1, 384); // size
	_maps->SetTabPos(2, 448); // theme
	_maps->SetCurSel(_maps->GetData()->FindItem(conf.cl_map.Get()), false);
	_maps->SetScrollPos(_maps->GetCurSel() - (_maps->GetNumLinesVisible() - 1) * 0.5f);
	AddFront(_maps);

	SetFocus(_maps);


	//
	// settings
	//

	{
		float y =  16;

		_nightMode = std::make_shared<UI::CheckBox>(manager, texman);
		_nightMode->Move(x3, y);
		_nightMode->SetText(texman, _lang.night_mode.Get());
		_nightMode->SetCheck(conf.cl_nightmode.Get());
		AddFront(_nightMode);

		UI::Text::Create(this, texman, x3, y+=30, _lang.game_speed.Get(), alignTextLT);
		_gameSpeed = std::make_shared<UI::Edit>(manager, texman);
		_gameSpeed->Move(x3 + 20, y += 15);
		_gameSpeed->SetWidth(80);
		_gameSpeed->SetInt(conf.cl_speed.GetInt());
		AddFront(_gameSpeed);

		UI::Text::Create(this, texman, x3, y+=30, _lang.frag_limit.Get(), alignTextLT);
		_fragLimit = std::make_shared<UI::Edit>(manager, texman);
		_fragLimit->Move(x3 + 20, y += 15);
		_fragLimit->SetWidth(80);
		_fragLimit->SetInt(conf.cl_fraglimit.GetInt());
		AddFront(_fragLimit);

		UI::Text::Create(this, texman, x3, y+=30, _lang.time_limit.Get(), alignTextLT);
		_timeLimit = std::make_shared<UI::Edit>(manager, texman);
		_timeLimit->Move(x3 + 20, y += 15);
		_timeLimit->SetWidth(80);
		_timeLimit->SetInt(conf.cl_timelimit.GetInt());
		AddFront(_timeLimit);

		UI::Text::Create(this, texman, x3+30, y+=30, _lang.zero_no_limits.Get(), alignTextLT);
	}



	//
	// player list
	//

	UI::Text::Create(this, texman, 16, 240, _lang.human_player_list.Get(), alignTextLT);

	_players = std::make_shared<DefaultListBox>(manager, texman);
	_players->Move(x1, 256);
	_players->Resize(x2-x1, 96);
	_players->SetTabPos(0,   4); // name
	_players->SetTabPos(1, 192); // skin
	_players->SetTabPos(2, 256); // class
	_players->SetTabPos(3, 320); // team
	_players->eventChangeCurSel = std::bind(&NewGameDlg::OnSelectPlayer, this, std::placeholders::_1);
	AddFront(_players);


	UI::Text::Create(this, texman, 16, 368, _lang.AI_player_list.Get(), alignTextLT);
	_bots = std::make_shared<DefaultListBox>(manager, texman);
	_bots->Move(x1, 384);
	_bots->Resize(x2-x1, 96);
	_bots->SetTabPos(0,   4); // name
	_bots->SetTabPos(1, 192); // skin
	_bots->SetTabPos(2, 256); // class
	_bots->SetTabPos(3, 320); // team
	_bots->eventChangeCurSel = std::bind(&NewGameDlg::OnSelectBot, this, std::placeholders::_1);
	AddFront(_bots);


	//
	// buttons
	//

	{
		std::shared_ptr<UI::Button> btn;


		btn = UI::Button::Create(this, texman, _lang.human_player_add.Get(), x3, 256);
		btn->eventClick = std::bind(&NewGameDlg::OnAddPlayer, this);

		_removePlayer = UI::Button::Create(this, texman, _lang.human_player_remove.Get(), x3, 286);
		_removePlayer->eventClick = std::bind(&NewGameDlg::OnRemovePlayer, this);
		_removePlayer->SetEnabled(false);

		_changePlayer = UI::Button::Create(this, texman, _lang.human_player_modify.Get(), x3, 316);
		_changePlayer->eventClick = std::bind(&NewGameDlg::OnEditPlayer, this);
		_changePlayer->SetEnabled(false);

		btn = UI::Button::Create(this, texman, _lang.AI_player_add.Get(), x3, 384);
		btn->eventClick = std::bind(&NewGameDlg::OnAddBot, this);

		_removeBot = UI::Button::Create(this, texman, _lang.AI_player_remove.Get(), x3, 414);
		_removeBot->eventClick = std::bind(&NewGameDlg::OnRemoveBot, this);
		_removeBot->SetEnabled(false);

		_changeBot = UI::Button::Create(this, texman, _lang.AI_player_modify.Get(), x3, 444);
		_changeBot->eventClick = std::bind(&NewGameDlg::OnEditBot, this);
		_changeBot->SetEnabled(false);



		btn = UI::Button::Create(this, texman, _lang.dm_ok.Get(), 544, 510);
		btn->eventClick = std::bind(&NewGameDlg::OnOK, this);

		btn = UI::Button::Create(this, texman, _lang.dm_cancel.Get(), 656, 510);
		btn->eventClick = std::bind(&NewGameDlg::OnCancel, this);
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
	int selected = _players->GetCurSel();
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

	_players->SetCurSel(std::min(selected, _players->GetData()->GetItemCount()-1));
}

void NewGameDlg::RefreshBotsList()
{
	int selected = _bots->GetCurSel();
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

	_bots->SetCurSel(std::min(selected, _bots->GetData()->GetItemCount() - 1));
}

void NewGameDlg::OnAddPlayer()
{
	std::vector<std::string> skinNames;
	_texman.GetTextureNames(skinNames, "skin/", true);

	ConfVarTable &p = _conf.dm_players.PushBack(ConfVar::typeTable).AsTable();
	p.SetStr("skin", skinNames[rand() % skinNames.size()]);

	_newPlayer = true;
	auto dlg = std::make_shared<EditPlayerDlg>(GetManager(), _texman, p, _conf, _lang);
	dlg->eventClose = std::bind(&NewGameDlg::OnAddPlayerClose, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
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
	assert( -1 != _players->GetCurSel() );
	_conf.dm_players.RemoveAt(_players->GetCurSel());
	RefreshPlayersList();
}

void NewGameDlg::OnEditPlayer()
{
	int index = _players->GetCurSel();
	assert(-1 != index);

	auto dlg = std::make_shared<EditPlayerDlg>(GetManager(), _texman, _conf.dm_players.GetAt(index).AsTable(), _conf, _lang);
	dlg->eventClose = std::bind(&NewGameDlg::OnEditPlayerClose, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
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
	_texman.GetTextureNames(skinNames, "skin/", true);

	ConfVarTable &p = _conf.dm_bots.PushBack(ConfVar::typeTable).AsTable();
	p.SetStr("skin", skinNames[rand() % skinNames.size()]);

	_newPlayer = true;
	auto dlg = std::make_shared<EditBotDlg>(GetManager(), _texman, p, _lang);
	dlg->eventClose = std::bind(&NewGameDlg::OnAddBotClose, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
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
	assert( -1 != _bots->GetCurSel() );
	_conf.dm_bots.RemoveAt(_bots->GetCurSel());
	RefreshBotsList();
}

void NewGameDlg::OnEditBot()
{
	int index = _bots->GetCurSel();
	assert(-1 != index);

	auto dlg = std::make_shared<EditBotDlg>(GetManager(), _texman, _conf.dm_bots.GetAt(index).AsTable(), _lang);
	dlg->eventClose = std::bind(&NewGameDlg::OnEditBotClose, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
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
	int index = _maps->GetCurSel();
	if( -1 != index )
	{
		fn = _maps->GetData()->GetItemText(index, 0);
	}
	else
	{
		return;
	}

	_conf.cl_speed.SetInt(std::max(MIN_GAMESPEED, std::min(MAX_GAMESPEED, _gameSpeed->GetInt())));
	_conf.cl_fraglimit.SetInt(std::max(0, std::min(MAX_FRAGLIMIT, _fragLimit->GetInt())));
	_conf.cl_timelimit.SetInt(std::max(0, std::min(MAX_TIMELIMIT, _timeLimit->GetInt())));
	_conf.cl_nightmode.Set(_nightMode->GetCheck());

	_conf.sv_speed.SetInt(_conf.cl_speed.GetInt());
	_conf.sv_fraglimit.SetInt(_conf.cl_fraglimit.GetInt());
	_conf.sv_timelimit.SetInt(_conf.cl_timelimit.GetInt());
	_conf.sv_nightmode.Set(_conf.cl_nightmode.Get());

	_conf.cl_map.Set(fn);
	_conf.ui_showmsg.Set(true);

	Close(_resultOK);
}

void NewGameDlg::OnCancel()
{
	Close(_resultCancel);
}

void NewGameDlg::OnSelectPlayer(int index)
{
	_removePlayer->SetEnabled( -1 != index );
	_changePlayer->SetEnabled( -1 != index );
}

void NewGameDlg::OnSelectBot(int index)
{
	_removeBot->SetEnabled( -1 != index );
	_changeBot->SetEnabled( -1 != index );
}

bool NewGameDlg::OnKeyPressed(UI::InputContext &ic, UI::Key key)
{
	switch(key)
	{
	case UI::Key::Enter:
	case UI::Key::NumEnter:
		if( GetFocus() == _players && -1 != _players->GetCurSel() )
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

EditPlayerDlg::EditPlayerDlg(UI::LayoutManager &manager, TextureManager &texman, ConfVarTable &info, ConfCache &conf, LangCache &lang)
  : Dialog(manager, texman, 384, 220)
  , _info(&info)
{
	SetEasyMove(true);

	auto title = UI::Text::Create(this, texman, GetWidth() / 2, 16, lang.player_settings.Get(), alignTextCT);
	title->SetFont(texman, "font_default");

	float x1 = 30;
	float x2 = x1 + 56;
	float y = 56;

	_skinPreview = std::make_shared<Window>(manager);
	_skinPreview->Move(300, y);
	_skinPreview->SetTexture(texman, nullptr, false);
	AddFront(_skinPreview);


	//
	// player name field
	//

	UI::Text::Create(this, texman, x1, y, lang.player_nick.Get(), alignTextLT);
	_name = std::make_shared<UI::Edit>(manager, texman);
	_name->Move(x2, y -= 1);
	_name->SetWidth(200);
	_name->SetText(texman, _info.nick.Get() );
	AddFront(_name);
	SetFocus(_name);


	//
	// skins combo
	//
	UI::Text::Create(this, texman, x1, y+=24, lang.player_skin.Get(), alignTextLT);
	_skins = std::make_shared<DefaultComboBox>(manager, texman);
	_skins->Move(x2, y -= 1);
	_skins->Resize(200);
	_skins->eventChangeCurSel = std::bind(&EditPlayerDlg::OnChangeSkin, this, std::placeholders::_1);
	AddFront(_skins);
	std::vector<std::string> names;
	texman.GetTextureNames(names, "skin/", true);
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
	_skins->GetList()->AlignHeightToContent();


	//
	// create and fill the classes list
	//

	UI::Text::Create(this, texman, x1, y+=24, lang.player_class.Get(), alignTextLT);
	_classes = std::make_shared<DefaultComboBox>(manager, texman);
	_classes->Move(x2, y -= 1);
	_classes->Resize(200);
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
	_classes->GetList()->AlignHeightToContent();

	UI::Text::Create(this, texman, x1, y+=24, lang.player_team.Get(), alignTextLT);
	_teams = std::make_shared<DefaultComboBox>(manager, texman);
	_teams->Move(x2, y -= 1);
	_teams->Resize(200);
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
	_teams->GetList()->AlignHeightToContent();



	//
	// player profile combo
	//

	UI::Text::Create(this, texman, x1, y+=24, lang.player_profile.Get(), alignTextLT);
	_profiles = std::make_shared<DefaultComboBox>(manager, texman);
	_profiles->Move(x2, y -= 1);
	_profiles->Resize(200);
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
	_profiles->GetList()->AlignHeightToContent();


	//
	// create buttons
	//

	UI::Button::Create(this, texman, lang.common_ok.Get(), 176, 190)->eventClick = std::bind(&Dialog::Close, this, _resultOK);
	UI::Button::Create(this, texman, lang.common_cancel.Get(), 280, 190)->eventClick = std::bind(&Dialog::Close, this, _resultCancel);
}

void EditPlayerDlg::OnChangeSkin(int index)
{
	if( -1 != index )
	{
		_skinPreview->SetTexture(GetManager().GetTextureManager(), ("skin/" + _skins->GetData()->GetItemText(index, 0)).c_str(), true);
	}
}

bool EditPlayerDlg::OnClose(int result)
{
	_info.nick.Set(_name->GetText());
	_info.skin.Set(_skins->GetData()->GetItemText(_skins->GetCurSel(), 0));
	_info.platform_class.Set(_classes->GetData()->GetItemText(_classes->GetCurSel(), 0));
	_info.team.SetInt(_teams->GetCurSel());
	_info.profile.Set(_profiles->GetData()->GetItemText(_profiles->GetCurSel(), 0));

	return true;
}

///////////////////////////////////////////////////////////////////////////////

static const char s_levels[][16] = {
	"bot_level_0",
	"bot_level_1",
	"bot_level_2",
	"bot_level_3",
	"bot_level_4",
};


EditBotDlg::EditBotDlg(UI::LayoutManager &manager, TextureManager &texman, ConfVarTable &info, LangCache &lang)
  : Dialog(manager, texman, 384, 220)
  , _info(&info)
{
	SetEasyMove(true);

	auto title = UI::Text::Create(this, texman, GetWidth() / 2, 16, lang.bot_settings.Get(), alignTextCT);
	title->SetFont(texman, "font_default");


	float x1 = 30;
	float x2 = x1 + 56;
	float y = 56;

	_skinPreview = std::make_shared<Window>(manager);
	_skinPreview->Move(300, y);
	_skinPreview->SetTexture(texman, nullptr, false);
	AddFront(_skinPreview);


	//
	// player name field
	//

	UI::Text::Create(this, texman, x1, y, lang.player_nick.Get(), alignTextLT);
	_name = std::make_shared<UI::Edit>(manager, texman);
	_name->Move(x2, y -= 1);
	_name->SetWidth(200);
	_name->SetText(texman, _info.nick.Get().empty() ? "player" : _info.nick.Get());
	AddFront(_name);
	SetFocus(_name);


	//
	// skins combo
	//
	UI::Text::Create(this, texman, x1, y+=24, lang.player_skin.Get(), alignTextLT);
	_skins = std::make_shared<DefaultComboBox>(manager, texman);
	_skins->Move(x2, y -= 1);
	_skins->Resize(200);
	_skins->eventChangeCurSel = std::bind(&EditBotDlg::OnChangeSkin, this, std::placeholders::_1);
	AddFront(_skins);
	std::vector<std::string> names;
	texman.GetTextureNames(names, "skin/", true);
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
	_skins->GetList()->AlignHeightToContent();


	//
	// create and fill the classes list
	//

	UI::Text::Create(this, texman, x1, y+=24, lang.player_class.Get(), alignTextLT);
	_classes = std::make_shared<DefaultComboBox>(manager, texman);
	_classes->Move(x2, y -= 1);
	_classes->Resize(200);
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
	_classes->GetList()->AlignHeightToContent();


	UI::Text::Create(this, texman, x1, y+=24, lang.player_team.Get(), alignTextLT);
	_teams = std::make_shared<DefaultComboBox>(manager, texman);
	_teams->Move(x2, y -= 1);
	_teams->Resize(200);
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
	_teams->GetList()->AlignHeightToContent();


	//
	// create and fill the levels list
	//

	UI::Text::Create(this, texman, x1, y+=24, lang.bot_level.Get(), alignTextLT);
	_levels = std::make_shared<DefaultComboBox>(manager, texman);
	_levels->Move(x2, y -= 1);
	_levels->Resize(200);
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
	_levels->GetList()->AlignHeightToContent();


	//
	// create buttons
	//

	UI::Button::Create(this, texman, lang.common_ok.Get(), 176, 190)->eventClick = std::bind(&EditBotDlg::OnOK, this);
	UI::Button::Create(this, texman, lang.common_cancel.Get(), 280, 190)->eventClick = std::bind(&EditBotDlg::OnCancel, this);
}

void EditBotDlg::OnOK()
{
	_info.nick.Set(_name->GetText());
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
		_skinPreview->SetTexture(GetManager().GetTextureManager(), ("skin/" + _skins->GetData()->GetItemText(index, 0)).c_str(), true);
	}
}

///////////////////////////////////////////////////////////////////////////////

void ScriptMessageBox::OnButton1()
{
	eventSelect(1);
}

void ScriptMessageBox::OnButton2()
{
	eventSelect(2);
}

void ScriptMessageBox::OnButton3()
{
	eventSelect(3);
}

ScriptMessageBox::ScriptMessageBox(UI::LayoutManager &manager,
                                   TextureManager &texman,
                                    const std::string &title,
                                    const std::string &text,
                                    const std::string &btn1,
                                    const std::string &btn2,
                                    const std::string &btn3)
  : Window(manager)
{
	SetTexture(texman, "ui/window", false);
	SetDrawBorder(true);

	_text = UI::Text::Create(this, texman, 10, 10, text, alignTextLT);

	_button1 = UI::Button::Create(this, texman, btn1, 0, _text->GetHeight() + 20);
	_button1->eventClick = std::bind(&ScriptMessageBox::OnButton1, this);

	int nbtn = 1 + !btn2.empty() + !(btn3.empty() || btn2.empty());

	float by = _text->GetHeight() + 20;
	float bw = _button1->GetWidth();
	float w = std::max(_text->GetWidth() + 10, (bw + 10) * (float) nbtn) + 10;
	float h = by + _button1->GetHeight() + 10;


	Resize(w, h);

	_button1->Move(w - 10 - _button1->GetWidth(), _button1->GetY());


	if( !btn2.empty() )
	{
		_button2 = UI::Button::Create(this, texman, btn2, _button1->GetX() - 10 - bw, by);
		_button2->eventClick = std::bind(&ScriptMessageBox::OnButton2, this);
	}
	else
	{
		_button2.reset();
	}

	if( !btn2.empty() && !btn3.empty() )
	{
		_button3 = UI::Button::Create(this, texman, btn3, _button2->GetX() - 10 - bw, by);
		_button3->eventClick = std::bind(&ScriptMessageBox::OnButton3, this);
	}
	else
	{
		_button3.reset();
	}
}

