// gui.cpp

#include "stdafx.h"

#include "gui.h"
#include "gui_maplist.h"

#include "GuiManager.h"

#include "Button.h"
#include "List.h"
#include "Text.h"
#include "Edit.h"
#include "Combo.h"


#include "functions.h"
#include "level.h"
#include "macros.h"

#include "video/TextureManager.h"

#include "config/Config.h"
#include "config/Language.h"

#include "core/Console.h"

#include "fs/FileSystem.h"
#include "fs/MapFile.h"
#include "gc/Player.h"
#include "gc/ai.h"


namespace UI
{

///////////////////////////////////////////////////////////////////////////////

NewGameDlg::NewGameDlg(Window *parent)
  : Dialog(parent, 770, 550)
{
	PauseGame(true);

	_newPlayer = false;

	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 20;


	//
	// map list
	//

	new Text(this, 16, 16, g_lang->choose_map->Get(), alignTextLT);

	_maps = new MapList(this, x1, 32, x2 - x1, 192);
	GetManager()->SetFocusWnd(_maps);


	//
	// settings
	//

	{
		float y =  16;

		_nightMode = new CheckBox(this, x3, y, g_lang->night_mode->Get());
		_nightMode->SetCheck( g_conf->cl_nightmode->Get() );


		new Text(this, x3, y+=30, g_lang->game_speed->Get(), alignTextLT);
		_gameSpeed = new Edit(this, x3+20, y+=15, 80);
		_gameSpeed->SetInt(g_conf->cl_speed->GetInt());

		new Text(this, x3, y+=30, g_lang->frag_limit->Get(), alignTextLT);
		_fragLimit = new Edit(this, x3+20, y+=15, 80);
		_fragLimit->SetInt(g_conf->cl_fraglimit->GetInt());

		new Text(this, x3, y+=30, g_lang->time_limit->Get(), alignTextLT);
		_timeLimit = new Edit(this, x3+20, y+=15, 80);
		_timeLimit->SetInt(g_conf->cl_timelimit->GetInt());

		new Text(this, x3+30, y+=30, g_lang->zero_no_limits->Get(), alignTextLT);
	}



	//
	// player list
	//

	new Text(this, 16, 240, g_lang->human_player_list->Get(), alignTextLT);

	_players = new List(this, x1, 256, x2-x1, 96);
	_players->SetTabPos(0,   4); // name
	_players->SetTabPos(1, 192); // skin
	_players->SetTabPos(2, 256); // class
	_players->SetTabPos(3, 320); // team
	_players->eventChangeCurSel.bind(&NewGameDlg::OnSelectPlayer, this);


	new Text(this, 16, 368, g_lang->AI_player_list->Get(), alignTextLT);
	_bots = new List(this, x1, 384, x2-x1, 96);
	_bots->SetTabPos(0,   4); // name
	_bots->SetTabPos(1, 192); // skin
	_bots->SetTabPos(2, 256); // class
	_bots->SetTabPos(3, 320); // team
	_bots->eventChangeCurSel.bind(&NewGameDlg::OnSelectBot, this);


	//
	// buttons
	//

	{
		Button *btn;


		btn = new Button(this, x3, 256, g_lang->human_player_add->Get());
		btn->eventClick.bind(&NewGameDlg::OnAddPlayer, this);

		_removePlayer = new Button(this, x3, 286, g_lang->human_player_remove->Get());
		_removePlayer->eventClick.bind(&NewGameDlg::OnRemovePlayer, this);
		_removePlayer->Enable(false);

		_changePlayer = new Button(this, x3, 316, g_lang->human_player_modify->Get());
		_changePlayer->eventClick.bind(&NewGameDlg::OnEditPlayer, this);
		_changePlayer->Enable(false);

		btn = new Button(this, x3, 384, g_lang->AI_player_add->Get());
		btn->eventClick.bind(&NewGameDlg::OnAddBot, this);

		_removeBot = new Button(this, x3, 414, g_lang->AI_player_remove->Get());
		_removeBot->eventClick.bind(&NewGameDlg::OnRemoveBot, this);
		_removeBot->Enable(false);

		_changeBot = new Button(this, x3, 444, g_lang->AI_player_modify->Get());
		_changeBot->eventClick.bind(&NewGameDlg::OnEditBot, this);
		_changeBot->Enable(false);



		btn = new Button(this, 544, 510, g_lang->dm_ok->Get());
		btn->eventClick.bind(&NewGameDlg::OnOK, this);

		btn = new Button(this, 656, 510, g_lang->dm_cancel->Get());
		btn->eventClick.bind(&NewGameDlg::OnCancel, this);
	}

	// call this after creation of buttons
	RefreshPlayersList();
	RefreshBotsList();
}

NewGameDlg::~NewGameDlg()
{
	PauseGame(false);
}

void NewGameDlg::RefreshPlayersList()
{
	int selected = _players->GetCurSel();
	_players->DeleteAllItems();

	for( size_t i = 0; i < g_conf->dm_players->GetSize(); ++i )
	{
		ConfVarTable *p = g_conf->dm_players->GetAt(i)->AsTable();

		int index = _players->AddItem( p->GetStr("nick")->Get() );
		_players->SetItemText(index, 1, p->GetStr("skin")->Get());
		_players->SetItemText(index, 2, p->GetStr("class")->Get());

		char s[16];
		int team = p->GetNum("team", 0)->GetInt();
		if( 0 != team )
		{
			wsprintf(s, "%d", team);
		}
		else
		{
			wsprintf(s, g_lang->team_none->Get());
		}

		_players->SetItemText(index, 3, s);
	}

	_players->SetCurSel(__min(selected, _players->GetSize()-1));
}

void NewGameDlg::RefreshBotsList()
{
	int selected = _bots->GetCurSel();
	_bots->DeleteAllItems();

	for( size_t i = 0; i < g_conf->dm_bots->GetSize(); ++i )
	{
		ConfVarTable *p = g_conf->dm_bots->GetAt(i)->AsTable();

		int index = _bots->AddItem( p->GetStr("nick")->Get() );
		_bots->SetItemText(index, 1, p->GetStr("skin")->Get());
		_bots->SetItemText(index, 2, p->GetStr("class")->Get());

		char s[16];
		int team = p->GetNum("team", 0)->GetInt();
		if( 0 != team )
		{
			wsprintf(s, "%d", team);
		}
		else
		{
			wsprintf(s, g_lang->team_none->Get());
		}

		_bots->SetItemText(index, 3, s);
	}

	_bots->SetCurSel(__min(selected, _bots->GetSize()-1));
}

void NewGameDlg::OnAddPlayer()
{
	std::vector<string_t> skinNames;
	g_texman->GetTextureNames(skinNames, "skin/", true);

	ConfVarTable *p = g_conf->dm_players->PushBack(ConfVar::typeTable)->AsTable();
	p->SetStr("skin", skinNames[rand() % skinNames.size()].c_str());

	_newPlayer = true;
	(new EditPlayerDlg(this, p))->eventClose.bind( &NewGameDlg::OnAddPlayerClose, this );
}

void NewGameDlg::OnAddPlayerClose(int result)
{
	if( _resultOK == result )
	{
		RefreshPlayersList();
	}
	else if( _newPlayer )
	{
		g_conf->dm_players->PopBack();
	}
	_newPlayer = false;
}

void NewGameDlg::OnRemovePlayer()
{
	_ASSERT( -1 != _players->GetCurSel() );
	g_conf->dm_players->RemoveAt(_players->GetCurSel());
	RefreshPlayersList();
}

void NewGameDlg::OnEditPlayer()
{
	int index = _players->GetCurSel();
	_ASSERT(-1 != index);

	(new EditPlayerDlg(this, g_conf->dm_players->GetAt(index)->AsTable()))
		->eventClose.bind( &NewGameDlg::OnEditPlayerClose, this );
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
	std::vector<string_t> skinNames;
	g_texman->GetTextureNames(skinNames, "skin/", true);

	ConfVarTable *p = g_conf->dm_bots->PushBack(ConfVar::typeTable)->AsTable();
	p->SetStr("skin", skinNames[rand() % skinNames.size()].c_str());

	_newPlayer = true;
	(new EditBotDlg(this, p))->eventClose.bind( &NewGameDlg::OnAddBotClose, this );
}

void NewGameDlg::OnAddBotClose(int result)
{
	if( _resultOK == result )
	{
		RefreshBotsList();
	}
	else if( _newPlayer )
	{
		g_conf->dm_bots->PopBack();
	}
	_newPlayer = false;
}

void NewGameDlg::OnRemoveBot()
{
	_ASSERT( -1 != _bots->GetCurSel() );
	g_conf->dm_bots->RemoveAt(_bots->GetCurSel());
	RefreshBotsList();
}

void NewGameDlg::OnEditBot()
{
	int index = _bots->GetCurSel();
	_ASSERT(-1 != index);

	(new EditBotDlg(this, g_conf->dm_bots->GetAt(index)->AsTable()))
		->eventClose.bind( &NewGameDlg::OnEditBotClose, this );
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
	string_t fn;
	int index = _maps->GetCurSel();
	if( -1 != index )
	{
		fn = _maps->GetItemText(index, 0);
	}
	else
	{
//		MessageBoxT(NULL, "Выберите карту", MB_OK|MB_ICONHAND);
		return;
	}

	string_t path = DIR_MAPS;
	path += "\\";
	path += fn + ".map";

	g_conf->cl_speed->SetInt( __max(MIN_GAMESPEED, __min(MAX_GAMESPEED, _gameSpeed->GetInt())) );
	g_conf->cl_fraglimit->SetInt( __max(0, __min(MAX_FRAGLIMIT, _fragLimit->GetInt())) );
	g_conf->cl_timelimit->SetInt( __max(0, __min(MAX_TIMELIMIT, _timeLimit->GetInt())) );
	g_conf->cl_nightmode->Set( _nightMode->GetCheck() );

	g_conf->sv_speed->SetInt( g_conf->cl_speed->GetInt() );
	g_conf->sv_fraglimit->SetInt( g_conf->cl_fraglimit->GetInt() );
	g_conf->sv_timelimit->SetInt( g_conf->cl_timelimit->GetInt() );
	g_conf->sv_nightmode->Set( g_conf->cl_nightmode->Get() );

	script_exec(g_env.L, "reset()");
	_ASSERT(!g_level);
	g_level = new Level();

	if( g_level->init_newdm(path.c_str(), rand()) )
	{
		g_conf->cl_map->Set(fn.c_str());
		g_conf->ui_showmsg->Set(true);

		for( size_t i = 0; i < g_conf->dm_players->GetSize(); ++i )
		{
			ConfVarTable *p = g_conf->dm_players->GetAt(i)->AsTable();
			GC_PlayerLocal *player = new GC_PlayerLocal();
			player->SetTeam(    p->GetNum("team")->GetInt() );
			player->SetSkin(    p->GetStr("skin")->Get()    );
			player->SetClass(   p->GetStr("class")->Get()   );
			player->SetNick(    p->GetStr("nick")->Get()    );
			player->SetProfile( p->GetStr("profile")->Get() );
		}

		for( size_t i = 0; i < g_conf->dm_bots->GetSize(); ++i )
		{
			ConfVarTable *p = g_conf->dm_bots->GetAt(i)->AsTable();
			GC_PlayerAI *bot = new GC_PlayerAI();
			bot->SetTeam(  p->GetNum("team")->GetInt() );
			bot->SetSkin(  p->GetStr("skin")->Get()    );
			bot->SetClass( p->GetStr("class")->Get()   );
			bot->SetNick(  p->GetStr("nick")->Get()    );
			bot->SetLevel( p->GetNum("level", 2)->GetInt() );
		}
	}
	else
	{
		script_exec(g_env.L, "reset()");
//		MessageBoxT(NULL, "Ошибка при загрузке карты", MB_OK|MB_ICONHAND);
		return;
	}

	Close(_resultOK);
}

void NewGameDlg::OnCancel()
{
	Close(_resultCancel);
}

void NewGameDlg::OnSelectPlayer(int index)
{
	_removePlayer->Enable( -1 != index );
	_changePlayer->Enable( -1 != index );
}

void NewGameDlg::OnSelectBot(int index)
{
	_removeBot->Enable( -1 != index );
	_changeBot->Enable( -1 != index );
}

void NewGameDlg::OnRawChar(int c)
{
	switch(c)
	{
	case VK_RETURN:
		if( GetManager()->GetFocusWnd() == _players && -1 != _players->GetCurSel() )
			OnEditPlayer();
		else
			OnOK();
		break;
	case VK_INSERT:
		OnAddPlayer();
		break;
	default:
		Dialog::OnRawChar(c);
	}
}

///////////////////////////////////////////////////////////////////////////////

EditPlayerDlg::EditPlayerDlg(Window *parent, ConfVarTable *info)
  : Dialog(parent, 384, 220)
{
	SetEasyMove(true);
	_ASSERT(info);

	Text *title = new Text(this, GetWidth() / 2, 16, "Параметры игрока", alignTextCT);
	title->SetTexture("font_default");
	title->Resize(title->GetTextureWidth(), title->GetTextureHeight());


	_info = info;


	float x1 = 30;
	float x2 = x1 + 56;
	float y = 56;

	_skinPreview = new Window(this, 300, y, NULL);


	//
	// player name field
	//

	new Text(this, x1, y, "Имя", alignTextLT);
	_name = new Edit(this, x2, y-=1, 200);
	_name->SetText( _info->GetStr("nick", "Unnamed")->Get() );
	GetManager()->SetFocusWnd(_name);

	List *lst; // helps in combo box filling


	//
	// skins combo
	//
	new Text(this, x1, y+=24, "Скин", alignTextLT);
	_skins = new ComboBox(this, x2, y-=1, 200);
	_skins->eventChangeCurSel.bind( &EditPlayerDlg::OnChangeSkin, this );
	lst = _skins->GetList();
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, "skin/", true);
	for( size_t i = 0; i < names.size(); ++i )
	{
		int index = lst->AddItem( names[i].c_str() );
		if( names[i] == _info->GetStr("skin")->Get() )
		{
			_skins->SetCurSel(index);
		}
	}
	if( -1 == _skins->GetCurSel() )
	{
		_skins->SetCurSel(0);
	}
	lst->AlignHeightToContent();


	//
	// create and fill the classes list
	//

	new Text(this, x1, y+=24, "Класс", alignTextLT);
	_classes = new ComboBox(this, x2, y-=1, 200);

	std::pair<string_t, string_t> val;
	lua_getglobal(g_env.L, "classes");
	for( lua_pushnil(g_env.L); lua_next(g_env.L, -2); lua_pop(g_env.L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		val.first = lua_tostring(g_env.L, -2);
		val.second = lua_tostring(g_env.L, -2); //lua_tostring(L, -1);
		_classNames.push_back(val);

		if( std::string::npos == val.first.find('/') )
		{
			int index = _classes->GetList()->AddItem(val.first.c_str());
			if( val.first == _info->GetStr("class")->Get() )
			{
				_classes->SetCurSel(index);
			}
		}
	}
	if( -1 == _classes->GetCurSel() )
		_classes->SetCurSel(0);
	_classes->GetList()->AlignHeightToContent();

	new Text(this, x1, y+=24, "Команда", alignTextLT);
	_teams = new ComboBox(this, x2, y-=1, 200);
	lst = _teams->GetList();

	for( int i = 0; i < MAX_TEAMS; ++i )
	{
		char buf[8];
		wsprintf(buf, i ? "%u" : "[нет]", i);
		int index = lst->AddItem(buf);
		if( i == _info->GetNum("team")->GetInt() )
		{
			_teams->SetCurSel(index);
		}
	}
	if( -1 == _teams->GetCurSel() )
	{
		_teams->SetCurSel(0);
	}
	lst->AlignHeightToContent();



	//
	// player profile combo
	//

	new Text(this, x1, y+=24, "Профиль", alignTextLT);
	_profiles = new ComboBox(this, x2, y-=1, 200);
	lst = _profiles->GetList();

	std::vector<string_t> profiles;
	g_conf->dm_profiles->GetKeyList(profiles);

	for( size_t i = 0; i < profiles.size(); ++i )
	{
		int index = lst->AddItem(profiles[i].c_str());
		if( profiles[i] == _info->GetStr("profile")->Get() )
		{
			_profiles->SetCurSel(index);
		}
	}
	if( -1 == _profiles->GetCurSel() )
	{
		_profiles->SetCurSel(0);
	}
	lst->AlignHeightToContent();


	//
	// create buttons
	//

	(new Button(this, 176, 190, "OK"))->eventClick.bind(&EditPlayerDlg::OnOK, this);
	(new Button(this, 280, 190, "Отмена"))->eventClick.bind(&EditPlayerDlg::OnCancel, this);
}

void EditPlayerDlg::OnOK()
{
	_info->SetStr("nick",    _name->GetText().c_str() );
	_info->SetStr("skin",    _skins->GetList()->GetItemText(_skins->GetCurSel(), 0).c_str() );
	_info->SetStr("class",   _classes->GetList()->GetItemText(_classes->GetCurSel(), 0).c_str() );
	_info->SetNum("team",    _teams->GetCurSel());
	_info->SetStr("profile", _profiles->GetList()->GetItemText(_profiles->GetCurSel(), 0).c_str() );

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
		_skinPreview->SetTexture( ("skin/" + _skins->GetList()->GetItemText(index, 0)).c_str() );
		_skinPreview->Resize( _skinPreview->GetTextureWidth(), _skinPreview->GetTextureHeight() );
	}
}

///////////////////////////////////////////////////////////////////////////////

const char EditBotDlg::levels[][16] = {
	"Новичок",
	"Средний",
	"Бывалый",
	"Опытный",
	"Ветеран",
};


EditBotDlg::EditBotDlg(Window *parent, ConfVarTable *info)
  : Dialog(parent, 384, 220)
{
	SetEasyMove(true);
	_ASSERT(info);

	Text *title = new Text(this, GetWidth() / 2, 16, "Параметры бота", alignTextCT);
	title->SetTexture("font_default");
	title->Resize(title->GetTextureWidth(), title->GetTextureHeight());


	_info = info;


	float x1 = 30;
	float x2 = x1 + 56;
	float y = 56;

	_skinPreview = new Window(this, 300, y, NULL);


	//
	// player name field
	//

	new Text(this, x1, y, "Имя", alignTextLT);
	_name = new Edit(this, x2, y-=1, 200);
	lua_getglobal(g_env.L, "random_name");   // push function
	lua_call(g_env.L, 0, 1);
	_name->SetText( _info->GetStr("nick",
		lua_tostring(g_env.L, -1))->Get() ); // random default nick
	lua_pop(g_env.L, 1);                     // pop result
	GetManager()->SetFocusWnd(_name);



	List *lst; // helps in combo box filling


	//
	// skins combo
	//
	new Text(this, x1, y+=24, "Скин", alignTextLT);
	_skins = new ComboBox(this, x2, y-=1, 200);
	_skins->eventChangeCurSel.bind( &EditBotDlg::OnChangeSkin, this );
	lst = _skins->GetList();
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, "skin/", true);
	for( size_t i = 0; i < names.size(); ++i )
	{
		int index = lst->AddItem( names[i].c_str() );
		if( names[i] == _info->GetStr("skin")->Get() )
		{
			_skins->SetCurSel(index);
		}
	}
	if( -1 == _skins->GetCurSel() )
	{
		_skins->SetCurSel(0);
	}
	lst->AlignHeightToContent();


	//
	// create and fill the classes list
	//

	new Text(this, x1, y+=24, "Класс", alignTextLT);
	_classes= new ComboBox(this, x2, y-=1, 200);

	std::pair<string_t, string_t> val;
	lua_getglobal(g_env.L, "classes");
	for( lua_pushnil(g_env.L); lua_next(g_env.L, -2); lua_pop(g_env.L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		val.first = lua_tostring(g_env.L, -2);
		val.second = lua_tostring(g_env.L, -2); //lua_tostring(L, -1);
		_classNames.push_back(val);

		int index = _classes->GetList()->AddItem(val.first.c_str());
		if( val.first == _info->GetStr("class")->Get() )
		{
			_classes->SetCurSel(index);
		}
	}
	if( -1 == _classes->GetCurSel() )
		_classes->SetCurSel(0);
	_classes->GetList()->AlignHeightToContent();


	new Text(this, x1, y+=24, "Команда", alignTextLT);
	_teams = new ComboBox(this, x2, y-=1, 200);
	lst = _teams->GetList();

	for( int i = 0; i < MAX_TEAMS; ++i )
	{
		char buf[8];
		wsprintf(buf, i ? "%u" : "[нет]", i);
		int index = lst->AddItem(buf);
		if( i == _info->GetNum("team")->GetInt() )
		{
			_teams->SetCurSel(index);
		}
	}
	if( -1 == _teams->GetCurSel() )
	{
		_teams->SetCurSel(0);
	}
	lst->AlignHeightToContent();


	//
	// create and fill the levels list
	//

	new Text(this, x1, y+=24, "Уровень", alignTextLT);
	_levels = new ComboBox(this, x2, y-=1, 200);
	lst = _levels->GetList();

	for( int i = 0; i < 5; ++i )
	{
		int index = lst->AddItem(levels[i]);
		if( i == _info->GetNum("level", 2)->GetInt() )
		{
			_levels->SetCurSel(index);
		}
	}
	if( -1 == _levels->GetCurSel() )
	{
		_levels->SetCurSel(2);
	}
	lst->AlignHeightToContent();


	//
	// create buttons
	//

	(new Button(this, 176, 190, "OK"))->eventClick.bind(&EditBotDlg::OnOK, this);
	(new Button(this, 280, 190, "Отмена"))->eventClick.bind(&EditBotDlg::OnCancel, this);
}

void EditBotDlg::OnOK()
{
	_info->SetStr("nick",  _name->GetText().c_str() );
	_info->SetStr("skin",  _skins->GetList()->GetItemText(_skins->GetCurSel(), 0).c_str() );
	_info->SetStr("class", _classes->GetList()->GetItemText(_classes->GetCurSel(), 0).c_str() );
	_info->SetNum("team",  _teams->GetCurSel());
	_info->SetNum("level", _levels->GetCurSel());

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
		_skinPreview->SetTexture( ("skin/" + _skins->GetList()->GetItemText(index, 0)).c_str() );
		_skinPreview->Resize( _skinPreview->GetTextureWidth(), _skinPreview->GetTextureHeight() );
	}
}

///////////////////////////////////////////////////////////////////////////////

void ScriptMessageBox::RunScript(int btn)
{
	lua_rawgeti(g_env.L, LUA_REGISTRYINDEX, _handler);
	luaL_unref(g_env.L, LUA_REGISTRYINDEX, _handler);

	lua_pushinteger(g_env.L, btn);
	if( lua_pcall(g_env.L, 1, 0, 0) )
	{
		g_console->printf("%s\n", lua_tostring(g_env.L, -1));
		lua_pop(g_env.L, 1); // pop the error message from the stack
	}

	Destroy();
}

void ScriptMessageBox::OnButton1()
{
	RunScript(1);
}

void ScriptMessageBox::OnButton2()
{
	RunScript(2);
}

void ScriptMessageBox::OnButton3()
{
	RunScript(3);
}

ScriptMessageBox::ScriptMessageBox( Window *parent, int handler,
                                    const char *text,
                                    const char *btn1,
                                    const char *btn2,
                                    const char *btn3)
  : Window(parent)
{
	_ASSERT(text);
	_ASSERT(btn1);

	SetBorder(true);
	BringToBack();

	_handler = handler;
	_text = new Text(this, 10, 10, text, alignTextLT);

	_button1 = new Button(this, 0, _text->GetTextHeight() + 20, btn1);
	_button1->eventClick.bind(&ScriptMessageBox::OnButton1, this);

	int nbtn = 1 + (btn2 != NULL) + (btn3 != NULL);

	float by = _text->GetTextHeight() + 20;
	float bw = _button1->GetWidth();
	float w = __max(_text->GetTextWidth() + 10, (bw + 10) * (float) nbtn) + 10;
	float h = by + _button1->GetHeight() + 10;


	Resize(w, h);
	Move(ceilf((GetParent()->GetWidth() - w) / 2), ceilf((GetParent()->GetHeight() - h) / 2));

	_button1->Move(w - 10 - _button1->GetWidth(), _button1->GetY());


	if( btn2 )
	{
		_button2 = new Button(this, _button1->GetX() - 10 - bw, by, btn2);
		_button2->eventClick.bind(&ScriptMessageBox::OnButton2, this);
	}
	else
	{
		_button2 = NULL;
	}

	if( btn3 )
	{
		_button3 = new Button(this, _button2->GetX() - 10 - bw, by, btn3);
		_button3->eventClick.bind(&ScriptMessageBox::OnButton3, this);
	}
	else
	{
		_button3 = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
