// gui.cpp

#include "gui.h"

#include "Controller.h"
#include "globals.h"
#include "gui_maplist.h"
#include "InputManager.h"
#include "Macros.h"
#include "MapFile.h"
#include "script.h"

#include "video/TextureManager.h"

#include "config/Config.h"
#include "config/Language.h"

#include "core/debug.h"

#include "gc/Player.h"

#include "network/CommonTypes.h"

// ui
#include <Button.h>
#include <List.h>
#include <Text.h>
#include <Edit.h>
#include <Combo.h>
#include <DataSourceAdapters.h>
#include <GuiManager.h>


#include <FileSystem.h>
#include <GLFW/glfw3.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace UI
{

///////////////////////////////////////////////////////////////////////////////

NewGameDlg::NewGameDlg(Window *parent, Level &level, InputManager &inputMgr)
  : Dialog(parent, 770, 550)
  , _level(level)
  , _inputMgr(inputMgr)
{
	_newPlayer = false;

	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 20;


	//
	// map list
	//

	Text::Create(this, 16, 16, g_lang.choose_map.Get(), alignTextLT);

	_maps = MapList::Create(this);
	_maps->Move(x1, 32);
	_maps->Resize(x2 - x1, 192);
	_maps->SetTabPos(0,   4); // name
	_maps->SetTabPos(1, 384); // size
	_maps->SetTabPos(2, 448); // theme
	_maps->SetCurSel(_maps->GetData()->FindItem(g_conf.cl_map.Get()), false);
	_maps->SetScrollPos(_maps->GetCurSel() - (_maps->GetNumLinesVisible() - 1) * 0.5f);

	GetManager()->SetFocusWnd(_maps);


	//
	// settings
	//

	{
		float y =  16;

		_nightMode = CheckBox::Create(this, x3, y, g_lang.night_mode.Get());
		_nightMode->SetCheck( g_conf.cl_nightmode.Get() );


		Text::Create(this, x3, y+=30, g_lang.game_speed.Get(), alignTextLT);
		_gameSpeed = Edit::Create(this, x3+20, y+=15, 80);
		_gameSpeed->SetInt(g_conf.cl_speed.GetInt());

		Text::Create(this, x3, y+=30, g_lang.frag_limit.Get(), alignTextLT);
		_fragLimit = Edit::Create(this, x3+20, y+=15, 80);
		_fragLimit->SetInt(g_conf.cl_fraglimit.GetInt());

		Text::Create(this, x3, y+=30, g_lang.time_limit.Get(), alignTextLT);
		_timeLimit = Edit::Create(this, x3+20, y+=15, 80);
		_timeLimit->SetInt(g_conf.cl_timelimit.GetInt());

		Text::Create(this, x3+30, y+=30, g_lang.zero_no_limits.Get(), alignTextLT);
	}



	//
	// player list
	//

	Text::Create(this, 16, 240, g_lang.human_player_list.Get(), alignTextLT);

	_players = DefaultListBox::Create(this);
	_players->Move(x1, 256);
	_players->Resize(x2-x1, 96);
	_players->SetTabPos(0,   4); // name
	_players->SetTabPos(1, 192); // skin
	_players->SetTabPos(2, 256); // class
	_players->SetTabPos(3, 320); // team
	_players->eventChangeCurSel = std::bind(&NewGameDlg::OnSelectPlayer, this, std::placeholders::_1);


	Text::Create(this, 16, 368, g_lang.AI_player_list.Get(), alignTextLT);
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
		Button *btn;


		btn = Button::Create(this, g_lang.human_player_add.Get(), x3, 256);
		btn->eventClick = std::bind(&NewGameDlg::OnAddPlayer, this);

		_removePlayer = Button::Create(this, g_lang.human_player_remove.Get(), x3, 286);
		_removePlayer->eventClick = std::bind(&NewGameDlg::OnRemovePlayer, this);
		_removePlayer->SetEnabled(false);

		_changePlayer = Button::Create(this, g_lang.human_player_modify.Get(), x3, 316);
		_changePlayer->eventClick = std::bind(&NewGameDlg::OnEditPlayer, this);
		_changePlayer->SetEnabled(false);

		btn = Button::Create(this, g_lang.AI_player_add.Get(), x3, 384);
		btn->eventClick = std::bind(&NewGameDlg::OnAddBot, this);

		_removeBot = Button::Create(this, g_lang.AI_player_remove.Get(), x3, 414);
		_removeBot->eventClick = std::bind(&NewGameDlg::OnRemoveBot, this);
		_removeBot->SetEnabled(false);

		_changeBot = Button::Create(this, g_lang.AI_player_modify.Get(), x3, 444);
		_changeBot->eventClick = std::bind(&NewGameDlg::OnEditBot, this);
		_changeBot->SetEnabled(false);



		btn = Button::Create(this, g_lang.dm_ok.Get(), 544, 510);
		btn->eventClick = std::bind(&NewGameDlg::OnOK, this);

		btn = Button::Create(this, g_lang.dm_cancel.Get(), 656, 510);
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

	for( size_t i = 0; i < g_conf.dm_players.GetSize(); ++i )
	{
		ConfPlayerLocal p(g_conf.dm_players.GetAt(i)->AsTable());

		int index = _players->GetData()->AddItem(p.nick.Get());
		_players->GetData()->SetItemText(index, 1, p.skin.Get());
		_players->GetData()->SetItemText(index, 2, p.platform_class.Get());

		char s[16];
		int team = p.team.GetInt();
		if( 0 != team )
		{
			sprintf(s, "%d", team);
		}
		else
		{
			sprintf(s, "%s", g_lang.team_none.Get().c_str());
		}

		_players->GetData()->SetItemText(index, 3, s);
	}

	_players->SetCurSel(std::min(selected, _players->GetData()->GetItemCount()-1));
}

void NewGameDlg::RefreshBotsList()
{
	int selected = _bots->GetCurSel();
	_bots->GetData()->DeleteAllItems();

	for( size_t i = 0; i < g_conf.dm_bots.GetSize(); ++i )
	{
		ConfPlayerAI p(g_conf.dm_bots.GetAt(i)->AsTable());

		int index = _bots->GetData()->AddItem(p.nick.Get());
		_bots->GetData()->SetItemText(index, 1, p.skin.Get());
		_bots->GetData()->SetItemText(index, 2, p.platform_class.Get());

		char s[16];
		int team = p.team.GetInt();
		if( 0 != team )
		{
			sprintf(s, "%d", team);
		}
		else
		{
			sprintf(s, "%s", g_lang.team_none.Get().c_str());
		}

		_bots->GetData()->SetItemText(index, 3, s);
	}

	_bots->SetCurSel(std::min(selected, _bots->GetData()->GetItemCount() - 1));
}

void NewGameDlg::OnAddPlayer()
{
	std::vector<std::string> skinNames;
	g_texman->GetTextureNames(skinNames, "skin/", true);

	ConfVarTable *p = g_conf.dm_players.PushBack(ConfVar::typeTable)->AsTable();
	p->SetStr("skin", skinNames[rand() % skinNames.size()]);

	_newPlayer = true;
	(new EditPlayerDlg(this, p))->eventClose = std::bind(&NewGameDlg::OnAddPlayerClose, this, std::placeholders::_1);
}

void NewGameDlg::OnAddPlayerClose(int result)
{
	if( _resultOK == result )
	{
		RefreshPlayersList();
	}
	else if( _newPlayer )
	{
		g_conf.dm_players.PopBack();
	}
	_newPlayer = false;
}

void NewGameDlg::OnRemovePlayer()
{
	assert( -1 != _players->GetCurSel() );
	g_conf.dm_players.RemoveAt(_players->GetCurSel());
	RefreshPlayersList();
}

void NewGameDlg::OnEditPlayer()
{
	int index = _players->GetCurSel();
	assert(-1 != index);

	(new EditPlayerDlg(this, g_conf.dm_players.GetAt(index)->AsTable()))
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
	g_texman->GetTextureNames(skinNames, "skin/", true);

	ConfVarTable *p = g_conf.dm_bots.PushBack(ConfVar::typeTable)->AsTable();
	p->SetStr("skin", skinNames[rand() % skinNames.size()]);

	_newPlayer = true;
	(new EditBotDlg(this, p))->eventClose = std::bind(&NewGameDlg::OnAddBotClose, this, std::placeholders::_1);
}

void NewGameDlg::OnAddBotClose(int result)
{
	if( _resultOK == result )
	{
		RefreshBotsList();
	}
	else if( _newPlayer )
	{
		g_conf.dm_bots.PopBack();
	}
	_newPlayer = false;
}

void NewGameDlg::OnRemoveBot()
{
	assert( -1 != _bots->GetCurSel() );
	g_conf.dm_bots.RemoveAt(_bots->GetCurSel());
	RefreshBotsList();
}

void NewGameDlg::OnEditBot()
{
	int index = _bots->GetCurSel();
	assert(-1 != index);

	(new EditBotDlg(this, g_conf.dm_bots.GetAt(index)->AsTable()))
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

	std::string path = std::string(DIR_MAPS) + '/' + fn + ".map";

	g_conf.cl_speed.SetInt( std::max(MIN_GAMESPEED, std::min(MAX_GAMESPEED, _gameSpeed->GetInt())) );
	g_conf.cl_fraglimit.SetInt( std::max(0, std::min(MAX_FRAGLIMIT, _fragLimit->GetInt())) );
	g_conf.cl_timelimit.SetInt( std::max(0, std::min(MAX_TIMELIMIT, _timeLimit->GetInt())) );
	g_conf.cl_nightmode.Set( _nightMode->GetCheck() );

	g_conf.sv_speed.SetInt( g_conf.cl_speed.GetInt() );
	g_conf.sv_fraglimit.SetInt( g_conf.cl_fraglimit.GetInt() );
	g_conf.sv_timelimit.SetInt( g_conf.cl_timelimit.GetInt() );
	g_conf.sv_nightmode.Set( g_conf.cl_nightmode.Get() );

//	SAFE_DELETE(g_client);

	try
	{
        g_level->Clear();
        g_level->Seed(rand());
        g_level->Import(g_fs->Open(path)->QueryStream());
        if( !script_exec(g_env.L, g_level->_infoOnInit.c_str()) )
        {
            g_level->Clear();
            throw std::runtime_error("init script error");
        }
	}
	catch( const std::exception &e )
	{
		TRACE("could not load map - %s", e.what());
//		SAFE_DELETE(g_client);
		return;
	}

	g_conf.cl_map.Set(fn);
	g_conf.ui_showmsg.Set(true);

	for( size_t i = 0; i < g_conf.dm_players.GetSize(); ++i )
	{
		ConfPlayerLocal p(g_conf.dm_players.GetAt(i)->AsTable());
        
        GC_PlayerLocal *player = new GC_PlayerLocal();
        player->SetClass(p.platform_class.Get());
        player->SetNick(p.nick.Get());
        player->SetSkin(p.skin.Get());
        player->SetTeam(p.team.GetInt());
        player->UpdateSkin();
        
        _inputMgr.AssignController(player, p.profile.Get());
    }

	for( size_t i = 0; i < g_conf.dm_bots.GetSize(); ++i )
	{
		ConfPlayerAI p(g_conf.dm_bots.GetAt(i)->AsTable());
        GC_Player *ai = new GC_Player();
        ai->SetClass(p.platform_class.Get());
        ai->SetNick(p.nick.Get());
        ai->SetSkin(p.skin.Get());
        ai->SetTeam(p.team.GetInt());
//        ai->SetAILevel(std::max(0U, std::min(AI_MAX_LEVEL, p.level.GetInt())));
        ai->UpdateSkin();
        
	}

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

bool NewGameDlg::OnRawChar(int c)
{
	switch(c)
	{
	case GLFW_KEY_ENTER:
		if( GetManager()->GetFocusWnd() == _players && -1 != _players->GetCurSel() )
			OnEditPlayer();
		else
			OnOK();
		break;
	case GLFW_KEY_INSERT:
		OnAddPlayer();
		break;
	default:
		return Dialog::OnRawChar(c);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

EditPlayerDlg::EditPlayerDlg(Window *parent, ConfVarTable *info)
  : Dialog(parent, 384, 220)
  , _info(info)
{
	SetEasyMove(true);
	assert(info);

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang.player_settings.Get(), alignTextCT);
	title->SetFont("font_default");

	float x1 = 30;
	float x2 = x1 + 56;
	float y = 56;

	_skinPreview = Window::Create(this);
	_skinPreview->Move(300, y);
	_skinPreview->SetTexture(NULL, false);


	//
	// player name field
	//

	Text::Create(this, x1, y, g_lang.player_nick.Get(), alignTextLT);
	_name = Edit::Create(this, x2, y-=1, 200);
	_name->SetText( _info.nick.Get() );
	GetManager()->SetFocusWnd(_name);


	//
	// skins combo
	//
	Text::Create(this, x1, y+=24, g_lang.player_skin.Get(), alignTextLT);
	_skins = DefaultComboBox::Create(this);
	_skins->Move(x2, y -= 1);
	_skins->Resize(200);
	_skins->eventChangeCurSel = std::bind(&EditPlayerDlg::OnChangeSkin, this, std::placeholders::_1);
	std::vector<std::string> names;
	g_texman->GetTextureNames(names, "skin/", true);
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

	Text::Create(this, x1, y+=24, g_lang.player_class.Get(), alignTextLT);
	_classes = DefaultComboBox::Create(this);
	_classes->Move(x2, y -= 1);
	_classes->Resize(200);

	std::pair<std::string, std::string> val;
	lua_getglobal(g_env.L, "classes");
	for( lua_pushnil(g_env.L); lua_next(g_env.L, -2); lua_pop(g_env.L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		val.first = lua_tostring(g_env.L, -2);
		val.second = lua_tostring(g_env.L, -2); //lua_tostring(L, -1);
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

	Text::Create(this, x1, y+=24, g_lang.player_team.Get(), alignTextLT);
	_teams = DefaultComboBox::Create(this);
	_teams->Move(x2, y -= 1);
	_teams->Resize(200);
	for( int i = 0; i < MAX_TEAMS; ++i )
	{
		char buf[8];
		sprintf(buf, i ? "%u" : g_lang.team_none.Get().c_str(), i);
		int index = _teams->GetData()->AddItem(buf);
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

	Text::Create(this, x1, y+=24, g_lang.player_profile.Get(), alignTextLT);
	_profiles = DefaultComboBox::Create(this);
	_profiles->Move(x2, y -= 1);
	_profiles->Resize(200);
	std::vector<std::string> profiles;
	g_conf.dm_profiles.GetKeyList(profiles);

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

	Button::Create(this, g_lang.common_ok.Get(), 176, 190)->eventClick = std::bind(&EditPlayerDlg::OnOK, this);
	Button::Create(this, g_lang.common_cancel.Get(), 280, 190)->eventClick = std::bind(&EditPlayerDlg::OnCancel, this);
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

const char EditBotDlg::levels[][16] = {
	"bot_level_0",
	"bot_level_1",
	"bot_level_2",
	"bot_level_3",
	"bot_level_4",
};


EditBotDlg::EditBotDlg(Window *parent, ConfVarTable *info)
  : Dialog(parent, 384, 220)
  , _info(info)
{
	SetEasyMove(true);
	assert(info);

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang.bot_settings.Get(), alignTextCT);
	title->SetFont("font_default");


	float x1 = 30;
	float x2 = x1 + 56;
	float y = 56;

	_skinPreview = Window::Create(this);
	_skinPreview->Move(300, y);
	_skinPreview->SetTexture(NULL, false);


	//
	// player name field
	//

	Text::Create(this, x1, y, g_lang.player_nick.Get(), alignTextLT);
	_name = Edit::Create(this, x2, y-=1, 200);
	lua_getglobal(g_env.L, "random_name");   // push function
	lua_call(g_env.L, 0, 1);                 // random default nick
	_name->SetText(_info.nick.Get().empty() ? lua_tostring(g_env.L, -1) : _info.nick.Get());
	lua_pop(g_env.L, 1);                     // pop result
	GetManager()->SetFocusWnd(_name);


	//
	// skins combo
	//
	Text::Create(this, x1, y+=24, g_lang.player_skin.Get(), alignTextLT);
	_skins = DefaultComboBox::Create(this);
	_skins->Move(x2, y -= 1);
	_skins->Resize(200);
	_skins->eventChangeCurSel = std::bind(&EditBotDlg::OnChangeSkin, this, std::placeholders::_1);
	std::vector<std::string> names;
	g_texman->GetTextureNames(names, "skin/", true);
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

	Text::Create(this, x1, y+=24, g_lang.player_class.Get(), alignTextLT);
	_classes = DefaultComboBox::Create(this);
	_classes->Move(x2, y -= 1);
	_classes->Resize(200);

	std::pair<std::string, std::string> val;
	lua_getglobal(g_env.L, "classes");
	for( lua_pushnil(g_env.L); lua_next(g_env.L, -2); lua_pop(g_env.L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		val.first = lua_tostring(g_env.L, -2);
		val.second = lua_tostring(g_env.L, -2); //lua_tostring(L, -1);
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


	Text::Create(this, x1, y+=24, g_lang.player_team.Get(), alignTextLT);
	_teams = DefaultComboBox::Create(this);
	_teams->Move(x2, y -= 1);
	_teams->Resize(200);
	for( int i = 0; i < MAX_TEAMS; ++i )
	{
		char buf[8];
		sprintf(buf, i ? "%u" : g_lang.team_none.Get().c_str(), i);
		int index = _teams->GetData()->AddItem(buf);
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

	Text::Create(this, x1, y+=24, g_lang.bot_level.Get(), alignTextLT);
	_levels = DefaultComboBox::Create(this);
	_levels->Move(x2, y -= 1);
	_levels->Resize(200);

	for( int i = 0; i < 5; ++i )
	{
		int index = _levels->GetData()->AddItem(g_lang->GetRoot()->GetStr(levels[i], "")->Get());
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

	Button::Create(this, g_lang.common_ok.Get(), 176, 190)->eventClick = std::bind(&EditBotDlg::OnOK, this);
	Button::Create(this, g_lang.common_cancel.Get(), 280, 190)->eventClick = std::bind(&EditBotDlg::OnCancel, this);
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

	_text = Text::Create(this, 10, 10, text, alignTextLT);

	_button1 = Button::Create(this, btn1, 0, _text->GetHeight() + 20);
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
		_button2 = Button::Create(this, btn2, _button1->GetX() - 10 - bw, by);
		_button2->eventClick = std::bind(&ScriptMessageBox::OnButton2, this);

	}
	else
	{
		_button2 = NULL;
	}

	if( !btn2.empty() && !btn3.empty() )
	{
		_button3 = Button::Create(this, btn3, _button2->GetX() - 10 - bw, by);
		_button3->eventClick = std::bind(&ScriptMessageBox::OnButton3, this);
	}
	else
	{
		_button3 = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
