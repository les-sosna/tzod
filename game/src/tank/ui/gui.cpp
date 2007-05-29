// gui.cpp

#include "stdafx.h"
#include "gui.h"
#include "Button.h"
#include "List.h"
#include "Text.h"
#include "Edit.h"
#include "Combo.h"

#include "GuiManager.h"

#include "functions.h"
#include "options.h"
#include "level.h"
#include "macros.h"

#include "video/TextureManager.h"

#include "config/Config.h"

#include "fs/FileSystem.h"
#include "fs/MapFile.h"
#include "gc/Player.h"



namespace UI
{
///////////////////////////////////////////////////////////////////////////////

MainMenuDlg::MainMenuDlg(Window *parent) : Dialog(parent, 0, 0, 1, 1, true)
{
	SetBorder(false);
	SetTexture("gui_splash");
	Resize(GetTextureWidth(), GetTextureHeight());

	OnParentSize(parent->GetWidth(), parent->GetHeight());

	(new Button(this,   0, 256, "Игра (F2)"))->eventClick.bind(&MainMenuDlg::OnNewGame, this);
//	(new Button(this,  96, 256, "Загрузить"))->eventClick.bind(&MainMenuDlg::OnNewGame, this);
//	(new Button(this, 192, 256, "Сохранить"))->eventClick.bind(&MainMenuDlg::OnNewGame, this);
//	(new Button(this, 288, 256, "Настройки"))->eventClick.bind(&MainMenuDlg::OnNewGame, this);
	(new Button(this, 416, 256, "Выход (Alt+А4)"))->eventClick.bind(&MainMenuDlg::OnExit, this);
}

void MainMenuDlg::OnNewGame()
{
	NewGameDlg *dlg = new NewGameDlg(this);
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnExit()
{
	DestroyWindow(g_env.hMainWnd);
}

void MainMenuDlg::OnParentSize(float width, float height)
{
	Move( (width - GetWidth()) * 0.5f, (height - GetHeight()) * 0.5f );
}

void MainMenuDlg::OnCloseChild(int result)
{
	if( Dialog::_resultOK == result )
	{
		Close(result);
	}
}

void MainMenuDlg::OnRawChar(int c)
{
	switch(c)
	{
	case VK_F2:
		OnNewGame();
		break;
	default:
		Dialog::OnRawChar(c);
	}
}

///////////////////////////////////////////////////////////////////////////////

NewGameDlg::NewGameDlg(Window *parent)
  : Dialog(parent, 0, 0, 768, 512, true)
{
	Move( (parent->GetWidth() - GetWidth()) / 2,
		(parent->GetHeight() - GetHeight()) / 2 );


	//
	// map list
	//

	_maps = new List(this, 16, 16, 448, 224);
	_maps->SetTabPos(0,   4); // name
	_maps->SetTabPos(1, 256); // size
	_maps->SetTabPos(2, 320); // theme
	GetManager()->SetFocusWnd(_maps);


	SafePtr<IFileSystem> dir = g_fs->GetFileSystem(DIR_MAPS);
	if( dir )
	{
		std::set<string_t> files;
		if( dir->EnumAllFiles(files, TEXT("*.map")) )
		{
			int lastMapIndex = 0;

			for( std::set<string_t>::iterator it = files.begin(); it != files.end(); ++it )
			{
				string_t tmp = DIR_MAPS;
				tmp += "/";
				tmp += *it;

				MapFile file;
				if( file.Open(tmp.c_str(), false) )
				{
					it->erase(it->length() - 4); // cut out the file extension
					int index = _maps->AddItem(it->c_str());

					if( *it == g_conf.cl_map->Get() )
						lastMapIndex = index;

					char size[64];
					int h = 0, w = 0;
					file.getMapAttribute("width", w);
					file.getMapAttribute("height", h);
					wsprintf(size, "%3d*%d", w, h);
					_maps->SetItemText(index, 1, size);

					if( file.getMapAttribute("theme", tmp) )
					{
						_maps->SetItemText(index, 2, tmp.c_str());
					}
				}
			}

			_maps->SetCurSel(lastMapIndex, false);
			_maps->ScrollTo(lastMapIndex - (_maps->GetNumLinesVisible() - 1) * 0.5f);
		}
		else
		{
			_ASSERT(FALSE); // EnumAllFiles has returned error...
		}
	}


	//
	// settings
	//

	{
		float x = 480;
		float y =  16;

		_nightMode = new CheckBox(this, x, y, "Ночной режим");
		_nightMode->SetCheck( g_conf.cl_nightmode->Get() );


		new Text(this, x, y+=30, "Скорость игры, %", alignTextLT);
		_gameSpeed = new Edit(this, x+20, y+=15, 80);
		_gameSpeed->SetInt(g_conf.cl_speed->GetInt());

		new Text(this, x, y+=30, "Лимит фрагов", alignTextLT);
		_fragLimit = new Edit(this, x+20, y+=15, 80);
		_fragLimit->SetInt(g_conf.cl_fraglimit->GetInt());

		new Text(this, x, y+=30, "Лимит времени", alignTextLT);
		_timeLimit = new Edit(this, x+20, y+=15, 80);
		_timeLimit->SetInt(g_conf.cl_timelimit->GetInt());

		new Text(this, x+30, y+=40, "(0 - нет лимита)", alignTextLT);
	}



	//
	// player list
	//

	_players = new List(this, 16, 256, 448, 128);
	_players->SetTabPos(0,   4); // name
	_players->SetTabPos(1, 192); // skin
	_players->SetTabPos(2, 256); // class
	_players->SetTabPos(3, 320); // team
	_players->eventChangeCurSel.bind(&NewGameDlg::OnSelectPlayer, this);


	//
	// buttons
	//

	{
		Button *btn;

		btn = new Button(this, 480, 256, "Новый (Ins)");
		btn->eventClick.bind(&NewGameDlg::OnAddPlayer, this);

		_removePlayer = new Button(this, 480, 286, "Удалить");
		_removePlayer->eventClick.bind(&NewGameDlg::OnRemovePlayer, this);
		_removePlayer->Enable(false);

		_changePlayer = new Button(this, 480, 316, "Изменить");
		_changePlayer->eventClick.bind(&NewGameDlg::OnEditPlayer, this);
		_changePlayer->Enable(false);

		btn = new Button(this, 544, 472, "Поехали!");
		btn->eventClick.bind(&NewGameDlg::OnOK, this);

		btn = new Button(this, 656, 472, "Отмена");
		btn->eventClick.bind(&NewGameDlg::OnCancel, this);
	}


	// call this after creation of buttons
	RefreshPlayersList();
}

void NewGameDlg::RefreshPlayersList()
{
	int selected = _players->GetCurSel();

	_players->DeleteAllItems();

	for( int i = 0; i < OPT(dm_nPlayers); ++i )
	{
		int index = _players->AddItem(  OPT(dm_pdPlayers)[i].name);
		_players->SetItemText(index, 1, OPT(dm_pdPlayers)[i].skin);
		_players->SetItemText(index, 2, OPT(dm_pdPlayers)[i].cls);

		char s[16];
		if( 0 != OPT(dm_pdPlayers)[i].team )
			wsprintf(s, "%d", OPT(dm_pdPlayers)[i].team);
		else wsprintf(s, "[нет]");

		_players->SetItemText(index, 3, s);
	}

	_players->SetCurSel(__min(selected, _players->GetSize()-1));
}

void NewGameDlg::OnAddPlayer()
{
	_ASSERT(g_options.dm_nPlayers < MAX_PLAYERS);

	PlayerDesc &pd = g_options.dm_pdPlayers[g_options.dm_nPlayers];
	ZeroMemory(&pd, sizeof(PlayerDesc));

	std::vector<string_t> skinNames;
	g_texman->GetTextureNames(skinNames, "skin/", true);
	strcpy(pd.skin, skinNames[rand() % skinNames.size()].c_str());

	strcpy(pd.name, "new player");

	DWORD disablePlayers = 0;
	for( int i = 0; i < g_options.dm_nPlayers; ++i )
	{
		_ASSERT(-1 < g_options.dm_pdPlayers[i].type);
		disablePlayers |= (1 << g_options.dm_pdPlayers[i].type);
	}

	pd.type = -1;

	(new EditPlayerDlg(this, pd, disablePlayers))
		->eventClose.bind( &NewGameDlg::OnAddPlayerClose, this );
}

void NewGameDlg::OnAddPlayerClose(int result)
{
	if( _resultOK == result )
	{
		++g_options.dm_nPlayers;
		RefreshPlayersList();
	}
}

void NewGameDlg::OnRemovePlayer()
{
	_ASSERT( -1 != _players->GetCurSel() );

	int index = _players->GetCurSel();

	memmove(&g_options.dm_pdPlayers[index    ],
			&g_options.dm_pdPlayers[index + 1],
			sizeof(PlayerDesc) * (MAX_PLAYERS - g_options.dm_nPlayers) );
	g_options.dm_nPlayers--;

	RefreshPlayersList();
}

void NewGameDlg::OnEditPlayer()
{
	int index = _players->GetCurSel();
	_ASSERT(-1 != index);

	DWORD disablePlayers = 0;
	for( int i = 0; i < g_options.dm_nPlayers; ++i )
	{
		_ASSERT(-1 < g_options.dm_pdPlayers[i].type);
		if( index != i )
		{
			disablePlayers |= (1 << g_options.dm_pdPlayers[i].type);
		}
	}

	(new EditPlayerDlg(this, g_options.dm_pdPlayers[index], disablePlayers))
		->eventClose.bind( &NewGameDlg::OnEditPlayerClose, this );
}

void NewGameDlg::OnEditPlayerClose(int result)
{
	if( _resultOK == result )
	{
		RefreshPlayersList();
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

	g_conf.cl_speed->SetInt(__max(MIN_GAMESPEED, __min(MAX_GAMESPEED, _gameSpeed->GetInt())) );
	g_conf.cl_fraglimit->SetInt( __max(0, __min(MAX_FRAGLIMIT, _fragLimit->GetInt())) );
	g_conf.cl_timelimit->SetInt( __max(0, __min(MAX_TIMELIMIT, _timeLimit->GetInt())) );
	g_conf.cl_nightmode->Set( _nightMode->GetCheck() );

	g_conf.sv_speed->SetInt( g_conf.cl_speed->GetInt() );
	g_conf.sv_fraglimit->SetInt( g_conf.cl_fraglimit->GetInt() );
	g_conf.sv_timelimit->SetInt( g_conf.cl_timelimit->GetInt() );
	g_conf.sv_nightmode->Set( g_conf.cl_nightmode->Get() );

	SAFE_DELETE(g_level);
	g_level = new Level();

	if( g_level->init_newdm(path.c_str()) )
	{
		g_level->_seed = rand();
		g_conf.cl_map->Set(fn.c_str());

		// добавляем игроков в обратном порядке
		for( int i = OPT(dm_nPlayers) - 1; i >= 0; --i )
		{
			GC_Player *player = new GC_Player(g_options.dm_pdPlayers[i].team);
			player->SetController( g_options.dm_pdPlayers[i].type);
			player->_name  = g_options.dm_pdPlayers[i].name;
			player->_skin  = g_options.dm_pdPlayers[i].skin;
			player->_class = g_options.dm_pdPlayers[i].cls;
		}
	}
	else
	{
		SAFE_DELETE(g_level);
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

EditPlayerDlg::EditPlayerDlg(Window *parent, PlayerDesc &inout_desc, DWORD disablePlayers)
  : Dialog(parent, 0, 0, 384, 256), _playerDesc(inout_desc)
{
	Move((parent->GetWidth() - GetWidth()) * 0.5f,
	     (parent->GetHeight() - GetHeight()) * 0.5f);
	SetEasyMove(true);


	float x = 64;
	float y =  8;


	_skinPreview = new Window(this, 270, y, NULL);



	//
	// player name field
	//

	new Text(this, 8, y, "Имя", alignTextLT);
	_name = new Edit(this, x, y-=1, 200);
	_name->SetText(_playerDesc.name);





	List *lst; // helps in combo box filling


	//
	// player type combo
	//

	new Text(this, 8, y+=24, "Тип", alignTextLT);
	_types = new ComboBox(this, x, y-=1, 200);
	lst = _types->GetList();

	int index = 0;
	for( int type = 0; type < MAX_HUMANS; ++type )
	{
		if( !(disablePlayers & (1 << type)) )
		{
			char s[16];
			wsprintf(s, "человек %d", type+1);
			lst->AddItem(s, type);
			if( _playerDesc.type == type )
				_types->SetCurSel(index);
			++index;
		}
	}
	lst->AddItem("компьютер", MAX_HUMANS);
	if( -1 == _playerDesc.type )
		_types->SetCurSel(0);
	else if (MAX_HUMANS == _playerDesc.type)
		_types->SetCurSel(index);





	//
	// skins combo
	//
	new Text(this, 8, y+=24, "Скин", alignTextLT);
	_skins = new ComboBox(this, x, y-=1, 200);
	_skins->eventChangeCurSel.bind( &EditPlayerDlg::OnChangeSkin, this );
	lst = _skins->GetList();
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, "skin/", true);
	for( size_t i = 0; i < names.size(); ++i )
	{
		int index = lst->AddItem( names[i].c_str() );
		if( names[i] == _playerDesc.skin )
			_skins->SetCurSel(index);
	}
	if( -1 == _skins->GetCurSel() )
		_skins->SetCurSel(0);


	//
	// create and fill the classes list
	//

	new Text(this, 8, y+=24, "Класс", alignTextLT);
	_classesCombo = new ComboBox(this, x, y-=1, 200);

	std::pair<string_t, string_t> val;
	lua_State *L = g_env.hScript;
	lua_getglobal(L, "classes");
	for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
        val.first = lua_tostring(L, -2);
		val.second = lua_tostring(L, -2); //lua_tostring(L, -1);
		_classesNames.push_back(val);

		int index = _classesCombo->GetList()->AddItem(val.first.c_str());
		if( val.first == _playerDesc.cls )
		{
			_classesCombo->SetCurSel(index);
		}
	}
	if( -1 == _classesCombo->GetCurSel() )
		_classesCombo->SetCurSel(0);



	//
	// create buttons
	//

	(new Button(this, 176, 224, "OK"))->eventClick.bind(&EditPlayerDlg::OnOK, this);
	(new Button(this, 280, 224, "Отмена"))->eventClick.bind(&EditPlayerDlg::OnCancel, this);
}

void EditPlayerDlg::OnOK()
{
	_playerDesc.type = (short) _types->GetList()->GetItemData( _types->GetCurSel() );

	strcpy( _playerDesc.name, _name->GetText().c_str() );
	strcpy( _playerDesc.skin, _skins->GetList()->GetItemText(_skins->GetCurSel(), 0).c_str() );
	strcpy( _playerDesc.cls, _classesCombo->GetList()->GetItemText(_classesCombo->GetCurSel(), 0).c_str() );

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

SkinSelectorDlg::SkinSelectorDlg(Window *parent)
  : Dialog(parent, 0, 0, 512, 256)
{
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, "skin/", false);

	float x = 2;
	float y = 2;

	for( size_t i = 0; i < names.size(); ++i )
	{
		Window *wnd = new Window(this, x, y, names[i].c_str());
		x += wnd->GetWidth() + 4;
		if( x > GetWidth() )
		{
			y += wnd->GetHeight() + 4;
			x = 2;
		}
	}
}

void SkinSelectorDlg::OnOK()
{
	Close(_resultOK);
}

void SkinSelectorDlg::OnCancel()
{
	Close(_resultCancel);
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
