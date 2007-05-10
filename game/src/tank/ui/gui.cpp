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
			int lastMapIndex = -1;

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

					if( *it == OPT(cMapName) )
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

			_maps->SetCurSel(lastMapIndex, true);
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
		_nightMode->SetCheck( OPT(bNightMode) );


		new Text(this, x, y+=30, "Скорость игры, %", alignTextLT);
		_gameSpeed = new Edit(this, x+20, y+=15, 80);
		_gameSpeed->SetInt(OPT(gameSpeed));

		new Text(this, x, y+=30, "Лимит фрагов", alignTextLT);
		_fragLimit = new Edit(this, x+20, y+=15, 80);
		_fragLimit->SetInt(OPT(fraglimit));

		new Text(this, x, y+=30, "Лимит времени", alignTextLT);
		_timeLimit = new Edit(this, x+20, y+=15, 80);
		_timeLimit->SetInt(OPT(timelimit));

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
}

void NewGameDlg::OnAddPlayer()
{
	_ASSERT(g_options.dm_nPlayers < MAX_PLAYERS);

	PlayerDesc &pd = g_options.dm_pdPlayers[g_options.dm_nPlayers];
	ZeroMemory(&pd, sizeof(PlayerDesc));

	std::vector<string_t> skinNames;
	g_texman->GetTextureNames(skinNames, "skin/");
	strcpy(pd.skin, skinNames[rand() % skinNames.size()].c_str());

	strcpy(pd.name, "new player");

	DWORD disablePlayers = 0;
	for( int i = 0; i < g_options.dm_nPlayers; ++i )
	{
		_ASSERT(-1 < g_options.dm_pdPlayers[i].type);
		disablePlayers |= (1 << g_options.dm_pdPlayers[i].type);
	}

	pd.type = -1;

	new EditPlayerDlg(this, pd, disablePlayers);
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
//	PlayerDescEx pd;
//	new EditPlayerDlg(this, pde);
}

void NewGameDlg::OnClosePlayerDlg(int result)
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

	OPT(gameSpeed)  = __max(MIN_GAMESPEED, __min(MAX_GAMESPEED, _gameSpeed->GetInt()));
	OPT(fraglimit)  = __max(0, __min(MAX_FRAGLIMIT, _fragLimit->GetInt()));
	OPT(timelimit)  = __max(0, __min(MAX_TIMELIMIT, _timeLimit->GetInt()));
	OPT(bNightMode) = _nightMode->GetCheck();

	SAFE_DELETE(g_level);
	g_level = new Level();

	if( g_level->init_newdm(path.c_str()) )
	{
		g_level->_seed = rand();
		strcpy(OPT(cMapName), fn.c_str());

		// добавляем игроков в обратном порядке
		for( int i = OPT(dm_nPlayers) - 1; i >= 0; --i )
		{
			GC_Player *player = new GC_Player(g_options.dm_pdPlayers[i].team);
			player->SetController( g_options.dm_pdPlayers[i].type);
			strcpy(player->_name,  g_options.dm_pdPlayers[i].name);
			strcpy(player->_skin,  g_options.dm_pdPlayers[i].skin);
			strcpy(player->_class, g_options.dm_pdPlayers[i].cls);
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

void NewGameDlg::OnSelectPlayer()
{
	bool enable = -1 != _players->GetCurSel();
	_removePlayer->Enable( enable );
	_changePlayer->Enable( enable );
}

void NewGameDlg::OnRawChar(int c)
{
	switch(c)
	{
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



	new Text(this, 8, y, "Имя", alignTextLT);
	new Edit(this, x, y-=1, 200);



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
				lst->SetCurSel(index);
			++index;
		}
	}
	lst->AddItem("компьютер", MAX_HUMANS);
	if( -1 == _playerDesc.type )
		lst->SetCurSel(0);
	else if (MAX_HUMANS == _playerDesc.type)
		lst->SetCurSel(index);





	//
	// skins combo
	//
	new Text(this, 8, y+=24, "Скин", alignTextLT);
	_skins = new ComboBox(this, x, y-=1, 200);
	_skins->eventChangeCurSel.bind( &EditPlayerDlg::OnChangeSkin, this );
	lst = _skins->GetList();
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, TEXT("skin/"));
	for( size_t i = 0; i < names.size(); ++i )
	{
		lst->AddItem( names[i].c_str() );
	}


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
		
		_classesCombo->GetList()->AddItem(val.first.c_str());
		if( val.first == _playerDesc.cls )
		{
		//	SendDlgItemMessage(hDlg, IDC_CLASS, CB_SETCURSEL, index, 0);
		//	index = i;
		}
	}




	//
	// create buttons
	//

	(new Button(this, 176, 224, "OK"))->eventClick.bind(&EditPlayerDlg::OnOk, this);
	(new Button(this, 280, 224, "Отмена"))->eventClick.bind(&EditPlayerDlg::OnCancel, this);
}

void EditPlayerDlg::OnOk()
{
//	Close(_resultOK);

	new SkinSelectorDlg(this);

}

void EditPlayerDlg::OnCancel()
{
	Close(_resultCancel);
}

void EditPlayerDlg::OnChangeSkin()
{
	_skinPreview->SetTexture( _skins->GetList()->GetItemText(_skins->GetList()->GetCurSel(), 0).c_str() );
	_skinPreview->Resize( _skinPreview->GetTextureWidth(), _skinPreview->GetTextureHeight() );
}

///////////////////////////////////////////////////////////////////////////////

SkinSelectorDlg::SkinSelectorDlg(Window *parent)
  : Dialog(parent, 0, 0, 512, 256)
{
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, TEXT("skin/"));

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

void SkinSelectorDlg::OnOk()
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
