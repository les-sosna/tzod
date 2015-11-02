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


NewGameDlg::NewGameDlg(UI::Window *parent, FS::FileSystem &fs, ConfCache &conf, UI::ConsoleBuffer &logger, LangCache &lang)
  : Dialog(parent, 770, 550)
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

	UI::Text::Create(this, 16, 16, _lang.choose_map.Get(), alignTextLT);

	_maps = MapList::Create(this, fs, logger);
	_maps->Move(x1, 32);
	_maps->Resize(x2 - x1, 192);
	_maps->SetTabPos(0,   4); // name
	_maps->SetTabPos(1, 384); // size
	_maps->SetTabPos(2, 448); // theme
	_maps->SetCurSel(_maps->GetData()->FindItem(conf.cl_map.Get()), false);
	_maps->SetScrollPos(_maps->GetCurSel() - (_maps->GetNumLinesVisible() - 1) * 0.5f);

	GetManager().SetFocusWnd(_maps);


	//
	// settings
	//

	{
		float y =  16;

		_nightMode = UI::CheckBox::Create(this, x3, y, _lang.night_mode.Get());
		_nightMode->SetCheck(conf.cl_nightmode.Get());


		UI::Text::Create(this, x3, y+=30, _lang.game_speed.Get(), alignTextLT);
		_gameSpeed = UI::Edit::Create(this, x3+20, y+=15, 80);
		_gameSpeed->SetInt(conf.cl_speed.GetInt());

		UI::Text::Create(this, x3, y+=30, _lang.frag_limit.Get(), alignTextLT);
		_fragLimit = UI::Edit::Create(this, x3+20, y+=15, 80);
		_fragLimit->SetInt(conf.cl_fraglimit.GetInt());

		UI::Text::Create(this, x3, y+=30, _lang.time_limit.Get(), alignTextLT);
		_timeLimit = UI::Edit::Create(this, x3+20, y+=15, 80);
		_timeLimit->SetInt(conf.cl_timelimit.GetInt());

		UI::Text::Create(this, x3+30, y+=30, _lang.zero_no_limits.Get(), alignTextLT);
	}



	//
	// player list
	//

	UI::Text::Create(this, 16, 240, _lang.human_player_list.Get(), alignTextLT);

	_players = DefaultListBox::Create(this);
	_players->Move(x1, 256);
	_players->Resize(x2-x1, 96);
	_players->SetTabPos(0,   4); // name
	_players->SetTabPos(1, 192); // skin
	_players->SetTabPos(2, 256); // class
	_players->SetTabPos(3, 320); // team
	_players->eventChangeCurSel = std::bind(&NewGameDlg::OnSelectPlayer, this, std::placeholders::_1);


	UI::Text::Create(this, 16, 368, _lang.AI_player_list.Get(), alignTextLT);
	_bots = DefaultListBox::Create(this);
	_bots->Move(x1, 384);
	_bots->Resize(x2-x1, 96);
	_bots->SetTabPos(0,   4); // name
	_bots->SetTabPos(1, 192); // skin
	_bots->SetTabPos(2, 256); // class
	_bots->SetTabPos(3, 320); // team
	_bots->eventChangeCurSel = std::bind(&NewGameDlg::OnSelectBot, this, std::placeholders::_1);


	//
	// buttons
	//

	{
		UI::Button *btn;


		btn = UI::Button::Create(this, _lang.human_player_add.Get(), x3, 256);
		btn->eventClick = std::bind(&NewGameDlg::OnAddPlayer, this);

		_removePlayer = UI::Button::Create(this, _lang.human_player_remove.Get(), x3, 286);
		_removePlayer->eventClick = std::bind(&NewGameDlg::OnRemovePlayer, this);
		_removePlayer->SetEnabled(false);

		_changePlayer = UI::Button::Create(this, _lang.human_player_modify.Get(), x3, 316);
		_changePlayer->eventClick = std::bind(&NewGameDlg::OnEditPlayer, this);
		_changePlayer->SetEnabled(false);

		btn = UI::Button::Create(this, _lang.AI_player_add.Get(), x3, 384);
		btn->eventClick = std::bind(&NewGameDlg::OnAddBot, this);

		_removeBot = UI::Button::Create(this, _lang.AI_player_remove.Get(), x3, 414);
		_removeBot->eventClick = std::bind(&NewGameDlg::OnRemoveBot, this);
		_removeBot->SetEnabled(false);

		_changeBot = UI::Button::Create(this, _lang.AI_player_modify.Get(), x3, 444);
		_changeBot->eventClick = std::bind(&NewGameDlg::OnEditBot, this);
		_changeBot->SetEnabled(false);



		btn = UI::Button::Create(this, _lang.dm_ok.Get(), 544, 510);
		btn->eventClick = std::bind(&NewGameDlg::OnOK, this);

		btn = UI::Button::Create(this, _lang.dm_cancel.Get(), 656, 510);
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
	GetManager().GetTextureManager().GetTextureNames(skinNames, "skin/", true);

	ConfVarTable &p = _conf.dm_players.PushBack(ConfVar::typeTable).AsTable();
	p.SetStr("skin", skinNames[rand() % skinNames.size()]);

	_newPlayer = true;
	(new EditPlayerDlg(this, p, _conf, _lang))->eventClose = std::bind(&NewGameDlg::OnAddPlayerClose, this, std::placeholders::_1);
}

void NewGameDlg::OnAddPlayerClose(int result)
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

	(new EditPlayerDlg(this, _conf.dm_players.GetAt(index).AsTable(), _conf, _lang))
		->eventClose = std::bind(&NewGameDlg::OnEditPlayerClose, this, std::placeholders::_1);
}

void NewGameDlg::OnEditPlayerClose(int result)
{
	if( _resultOK == result )
	{
		RefreshPlayersList();
	}
}

void NewGameDlg::OnAddBot()
{
	std::vector<std::string> skinNames;
	GetManager().GetTextureManager().GetTextureNames(skinNames, "skin/", true);

	ConfVarTable &p = _conf.dm_bots.PushBack(ConfVar::typeTable).AsTable();
	p.SetStr("skin", skinNames[rand() % skinNames.size()]);

	_newPlayer = true;
	(new EditBotDlg(this, p, _lang))->eventClose = std::bind(&NewGameDlg::OnAddBotClose, this, std::placeholders::_1);
}

void NewGameDlg::OnAddBotClose(int result)
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

	(new EditBotDlg(this, _conf.dm_bots.GetAt(index).AsTable(), _lang))
		->eventClose = std::bind(&NewGameDlg::OnEditBotClose, this, std::placeholders::_1);
}

void NewGameDlg::OnEditBotClose(int result)
{
	if( _resultOK == result )
	{
		RefreshBotsList();
	}
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

bool NewGameDlg::OnKeyPressed(UI::Key key)
{
	switch(key)
	{
	case UI::Key::Enter:
		if( GetManager().GetFocusWnd() == _players && -1 != _players->GetCurSel() )
			OnEditPlayer();
		else
			OnOK();
		break;
	case UI::Key::Insert:
		OnAddPlayer();
		break;
	default:
		return Dialog::OnKeyPressed(key);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

EditPlayerDlg::EditPlayerDlg(Window *parent, ConfVarTable &info, ConfCache &conf, LangCache &lang)
  : Dialog(parent, 384, 220)
  , _info(&info)
{
	SetEasyMove(true);

	UI::Text *title = UI::Text::Create(this, GetWidth() / 2, 16, lang.player_settings.Get(), alignTextCT);
	title->SetFont("font_default");

	float x1 = 30;
	float x2 = x1 + 56;
	float y = 56;

	_skinPreview = Window::Create(this);
	_skinPreview->Move(300, y);
	_skinPreview->SetTexture(nullptr, false);


	//
	// player name field
	//

	UI::Text::Create(this, x1, y, lang.player_nick.Get(), alignTextLT);
	_name = UI::Edit::Create(this, x2, y-=1, 200);
	_name->SetText( _info.nick.Get() );
	GetManager().SetFocusWnd(_name);


	//
	// skins combo
	//
	UI::Text::Create(this, x1, y+=24, lang.player_skin.Get(), alignTextLT);
	_skins = DefaultComboBox::Create(this);
	_skins->Move(x2, y -= 1);
	_skins->Resize(200);
	_skins->eventChangeCurSel = std::bind(&EditPlayerDlg::OnChangeSkin, this, std::placeholders::_1);
	std::vector<std::string> names;
	GetManager().GetTextureManager().GetTextureNames(names, "skin/", true);
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

	UI::Text::Create(this, x1, y+=24, lang.player_class.Get(), alignTextLT);
	_classes = DefaultComboBox::Create(this);
	_classes->Move(x2, y -= 1);
	_classes->Resize(200);

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

	UI::Text::Create(this, x1, y+=24, lang.player_team.Get(), alignTextLT);
	_teams = DefaultComboBox::Create(this);
	_teams->Move(x2, y -= 1);
	_teams->Resize(200);
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

	UI::Text::Create(this, x1, y+=24, lang.player_profile.Get(), alignTextLT);
	_profiles = DefaultComboBox::Create(this);
	_profiles->Move(x2, y -= 1);
	_profiles->Resize(200);
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

	UI::Button::Create(this, lang.common_ok.Get(), 176, 190)->eventClick = std::bind(&EditPlayerDlg::OnOK, this);
	UI::Button::Create(this, lang.common_cancel.Get(), 280, 190)->eventClick = std::bind(&EditPlayerDlg::OnCancel, this);
}

void EditPlayerDlg::OnOK()
{
	_info.nick.Set(_name->GetText());
	_info.skin.Set(_skins->GetData()->GetItemText(_skins->GetCurSel(), 0) );
	_info.platform_class.Set(_classes->GetData()->GetItemText(_classes->GetCurSel(), 0) );
	_info.team.SetInt(_teams->GetCurSel());
	_info.profile.Set(_profiles->GetData()->GetItemText(_profiles->GetCurSel(), 0) );

	Close(_resultOK);
}

void EditPlayerDlg::OnCancel()
{
	Close(_resultCancel);
}

void EditPlayerDlg::OnChangeSkin(int index)
{
	if( -1 != index )
	{
		_skinPreview->SetTexture(("skin/" + _skins->GetData()->GetItemText(index, 0)).c_str(), true);
	}
}

///////////////////////////////////////////////////////////////////////////////

static const char s_levels[][16] = {
	"bot_level_0",
	"bot_level_1",
	"bot_level_2",
	"bot_level_3",
	"bot_level_4",
};


EditBotDlg::EditBotDlg(Window *parent, ConfVarTable &info, LangCache &lang)
  : Dialog(parent, 384, 220)
  , _info(&info)
{
	SetEasyMove(true);

	UI::Text *title = UI::Text::Create(this, GetWidth() / 2, 16, lang.bot_settings.Get(), alignTextCT);
	title->SetFont("font_default");


	float x1 = 30;
	float x2 = x1 + 56;
	float y = 56;

	_skinPreview = Window::Create(this);
	_skinPreview->Move(300, y);
	_skinPreview->SetTexture(nullptr, false);


	//
	// player name field
	//

	UI::Text::Create(this, x1, y, lang.player_nick.Get(), alignTextLT);
	_name = UI::Edit::Create(this, x2, y-=1, 200);
	_name->SetText(_info.nick.Get().empty() ? "player" : _info.nick.Get());
	GetManager().SetFocusWnd(_name);


	//
	// skins combo
	//
	UI::Text::Create(this, x1, y+=24, lang.player_skin.Get(), alignTextLT);
	_skins = DefaultComboBox::Create(this);
	_skins->Move(x2, y -= 1);
	_skins->Resize(200);
	_skins->eventChangeCurSel = std::bind(&EditBotDlg::OnChangeSkin, this, std::placeholders::_1);
	std::vector<std::string> names;
	GetManager().GetTextureManager().GetTextureNames(names, "skin/", true);
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

	UI::Text::Create(this, x1, y+=24, lang.player_class.Get(), alignTextLT);
	_classes = DefaultComboBox::Create(this);
	_classes->Move(x2, y -= 1);
	_classes->Resize(200);

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


	UI::Text::Create(this, x1, y+=24, lang.player_team.Get(), alignTextLT);
	_teams = DefaultComboBox::Create(this);
	_teams->Move(x2, y -= 1);
	_teams->Resize(200);
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

	UI::Text::Create(this, x1, y+=24, lang.bot_level.Get(), alignTextLT);
	_levels = DefaultComboBox::Create(this);
	_levels->Move(x2, y -= 1);
	_levels->Resize(200);

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

	UI::Button::Create(this, lang.common_ok.Get(), 176, 190)->eventClick = std::bind(&EditBotDlg::OnOK, this);
	UI::Button::Create(this, lang.common_cancel.Get(), 280, 190)->eventClick = std::bind(&EditBotDlg::OnCancel, this);
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
		_skinPreview->SetTexture(("skin/" + _skins->GetData()->GetItemText(index, 0)).c_str(), true);
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

ScriptMessageBox::ScriptMessageBox( Window *parent,
                                    const std::string &title,
                                    const std::string &text,
                                    const std::string &btn1,
                                    const std::string &btn2,
                                    const std::string &btn3)
  : Window(parent)
{
	SetTexture("ui/window", false);
	SetDrawBorder(true);
	BringToBack();

	_text = UI::Text::Create(this, 10, 10, text, alignTextLT);

	_button1 = UI::Button::Create(this, btn1, 0, _text->GetHeight() + 20);
	_button1->eventClick = std::bind(&ScriptMessageBox::OnButton1, this);

	int nbtn = 1 + !btn2.empty() + !(btn3.empty() || btn2.empty());

	float by = _text->GetHeight() + 20;
	float bw = _button1->GetWidth();
	float w = std::max(_text->GetWidth() + 10, (bw + 10) * (float) nbtn) + 10;
	float h = by + _button1->GetHeight() + 10;


	Resize(w, h);
	Move(ceil((GetParent()->GetWidth() - w) / 2), ceil((GetParent()->GetHeight() - h) / 2));

	_button1->Move(w - 10 - _button1->GetWidth(), _button1->GetY());


	if( !btn2.empty() )
	{
		_button2 = UI::Button::Create(this, btn2, _button1->GetX() - 10 - bw, by);
		_button2->eventClick = std::bind(&ScriptMessageBox::OnButton2, this);
	}
	else
	{
		_button2 = nullptr;
	}

	if( !btn2.empty() && !btn3.empty() )
	{
		_button3 = UI::Button::Create(this, btn3, _button2->GetX() - 10 - bw, by);
		_button3->eventClick = std::bind(&ScriptMessageBox::OnButton3, this);
	}
	else
	{
		_button3 = nullptr;
	}
}

