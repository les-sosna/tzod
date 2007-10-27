// gui_network.cpp

#include "stdafx.h"

#include "gui_network.h"
#include "gui_maplist.h"

#include "GuiManager.h"

#include "Text.h"
#include "Edit.h"
#include "Button.h"

#include "config/Config.h"

#include "Interface.h"
#include "functions.h"
#include "Level.h"
#include "Macros.h"

#include "network/TankServer.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

CreateServerDlg::CreateServerDlg(Window *parent)
  : Dialog(parent, 0, 0, 770, 450)
{
	Move( (parent->GetWidth() - GetWidth()) / 2, (parent->GetHeight() - GetHeight()) / 2 );


	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 20;
	float x4 = x3 + 20;

	//
	// map list
	//

	new Text(this, x1, 26, "Выберите карту", alignTextLT);

	_maps = new MapList(this, x1, 42, x2 - x1, 300);
	GetManager()->SetFocusWnd(_maps);


	//
	// settings
	//

	{
		float y =  16;

		_nightMode = new CheckBox(this, x3, y, "Ночной режим");
		_nightMode->SetCheck( g_conf.cl_nightmode->Get() );


		new Text(this, x3, y+=30, "Скорость игры, %", alignTextLT);
		_gameSpeed = new Edit(this, x4, y+=15, 80);
		_gameSpeed->SetInt(g_conf.cl_speed->GetInt());

		new Text(this, x3, y+=30, "Лимит фрагов", alignTextLT);
		_fragLimit = new Edit(this, x4, y+=15, 80);
		_fragLimit->SetInt(g_conf.cl_fraglimit->GetInt());

		new Text(this, x3, y+=30, "Лимит времени", alignTextLT);
		_timeLimit = new Edit(this, x4, y+=15, 80);
		_timeLimit->SetInt(g_conf.cl_timelimit->GetInt());

		new Text(this, x3+30, y+=30, "(0 - нет лимита)", alignTextLT);

		new Text(this, x3, y+=40, "Скорость сети, fps", alignTextLT);
		_svFps = new Edit(this, x4, y+=15, 100);
		_svFps->SetInt(g_conf.sv_fps->GetInt());

		new Text(this, x3, y+=30, "Задержка", alignTextLT);
		_svLatency = new Edit(this, x4, y+=15, 100);
		_svLatency->SetInt(g_conf.sv_latency->GetInt());
	}

	Button *btn;
	btn = new Button(this, 544, 410, "Создать");
	btn->eventClick.bind(&CreateServerDlg::OnOK, this);

	btn = new Button(this, 656, 410, "Отмена");
	btn->eventClick.bind(&CreateServerDlg::OnCancel, this);
}

CreateServerDlg::~CreateServerDlg()
{
}

void CreateServerDlg::OnOK()
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


	GAMEINFO gi = {0};
	gi.dwVersion  = VERSION;
	gi.dwMapCRC32 = CalcCRC32(path.c_str());
	gi.seed       = rand();
	gi.fraglimit  = __max(0, __min(MAX_FRAGLIMIT, _fragLimit->GetInt()));
	gi.timelimit  = __max(0, __min(MAX_TIMELIMIT, _timeLimit->GetInt()));
	gi.server_fps = __max(MIN_NETWORKSPEED, __min(MAX_NETWORKSPEED, _svFps->GetInt()));
	gi.latency    = __max(MIN_LATENCY, __min(MAX_LATENCY, _svLatency->GetInt()));
	gi.nightmode  = _nightMode->GetCheck();

	strcpy(gi.cMapName, fn.c_str());
	strcpy(gi.cServerName, "ZOD Server");

	script_exec(g_env.L, "reset()");
	_ASSERT(!g_level);
	g_level = new Level();
	g_level->_server = new TankServer();
	if( !g_level->_server->init(&gi) )
	{
		SAFE_DELETE(g_level);
		MessageBoxT(g_env.hMainWnd, "Не удалось запустить сервер. "
			"Проверьте конфигурацию сети", MB_OK|MB_ICONHAND);
		return;
	}

	g_conf.cl_map->Set(fn.c_str());

/*

	OPT(latency) = gi.latency;

	HWND hWndParent = GetParent(hDlg);
	EndDialog(hDlg, wmId);
	LOGOUT_1("DialogBox(IDD_CONNECT)\n");
	DialogBoxParam(g_hInstance, (LPCTSTR)IDD_CONNECT,
		hWndParent, (DLGPROC) dlgConnect, (LPARAM) "localhost");





	g_conf.cl_speed->SetInt( __max(MIN_GAMESPEED, __min(MAX_GAMESPEED, _gameSpeed->GetInt())) );
	g_conf.cl_fraglimit->SetInt( __max(0, __min(MAX_FRAGLIMIT, _fragLimit->GetInt())) );
	g_conf.cl_timelimit->SetInt( __max(0, __min(MAX_TIMELIMIT, _timeLimit->GetInt())) );
	g_conf.cl_nightmode->Set( _nightMode->GetCheck() );

	g_conf.sv_speed->SetInt( g_conf.cl_speed->GetInt() );
	g_conf.sv_fraglimit->SetInt( g_conf.cl_fraglimit->GetInt() );
	g_conf.sv_timelimit->SetInt( g_conf.cl_timelimit->GetInt() );
	g_conf.sv_nightmode->Set( g_conf.cl_nightmode->Get() );

	script_exec(g_env.L, "reset()");
	_ASSERT(!g_level);
	g_level = new Level();

	if( g_level->init_newdm(path.c_str()) )
	{
		g_conf.cl_map->Set(fn.c_str());

		for( size_t i = 0; i < g_conf.dm_players->GetSize(); ++i )
		{
			ConfVarTable *p = g_conf.dm_players->GetAt(i)->AsTable();
			GC_PlayerLocal *player = new GC_PlayerLocal();
			player->SetTeam(    p->GetNum("team")->GetInt() );
			player->SetSkin(    p->GetStr("skin")->Get()    );
			player->SetClass(   p->GetStr("class")->Get()   );
			player->SetNick(    p->GetStr("nick")->Get()    );
			player->SetProfile( p->GetStr("profile")->Get() );
		}

		for( size_t i = 0; i < g_conf.dm_bots->GetSize(); ++i )
		{
			ConfVarTable *p = g_conf.dm_bots->GetAt(i)->AsTable();
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
		SAFE_DELETE(g_level);
		//		MessageBoxT(NULL, "Ошибка при загрузке карты", MB_OK|MB_ICONHAND);
		return;
	}
*/

	new ConnectDlg(this);
//	Close(_resultOK);
}

void CreateServerDlg::OnCancel()
{
	Close(_resultCancel);
}


///////////////////////////////////////////////////////////////////////////////

ConnectDlg::ConnectDlg(Window *parent)
  : Dialog(parent, 0, 0, 512, 256)
{
	Move( (parent->GetWidth() - GetWidth()) / 2, (parent->GetHeight() - GetHeight()) / 2 );

	new Text(this, 16, 15, "Адрес сервера", alignTextLT);
	_name = new Edit(this, 20, 30, 300);
	_name->SetText(g_conf.cl_server->Get());

	_btnOK = new Button(this, 312, 226, "Соединение");
	_btnOK->eventClick.bind(&ConnectDlg::OnOK, this);

	(new Button(this, 412, 226, "Отмена"))->eventClick.bind(&ConnectDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_name);
}

ConnectDlg::~ConnectDlg()
{
}

void ConnectDlg::OnOK()
{
	_btnOK->Enable(false);
	_name->Enable(false);
}

void ConnectDlg::OnCancel()
{
	SAFE_DELETE(g_level);
	Close(_resultCancel);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
