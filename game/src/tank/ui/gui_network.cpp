// gui_network.cpp

#include "stdafx.h"

#include "gui_network.h"
#include "gui_maplist.h"

#include "GuiManager.h"

#include "Text.h"
#include "Edit.h"
#include "Button.h"
#include "Console.h"

#include "Interface.h"
#include "functions.h"
#include "Level.h"
#include "Macros.h"

#include "config/Config.h"

#include "core/Console.h"

#include "network/TankServer.h"

#include "gc/Player.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

CreateServerDlg::CreateServerDlg(Window *parent)
  : Dialog(parent, 770, 450)
{
	Text *title = new Text(this, GetWidth() / 2, 16, "Создать сервер", alignTextCT);
	title->SetTexture("font_default");
	title->Resize(title->GetTextureWidth(), title->GetTextureHeight());

	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 20;
	float x4 = x3 + 20;

	//
	// map list
	//

	new Text(this, x1, 46, "Выберите карту", alignTextLT);

	_maps = new MapList(this, x1, 62, x2 - x1, 300);
	GetManager()->SetFocusWnd(_maps);


	//
	// settings
	//

	{
		float y =  56;

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

	script_exec(g_env.L, "reset()");


	string_t path = DIR_MAPS;
	path += "\\";
	path += fn + ".map";

	GameInfo gi = {0};
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

	_ASSERT(NULL == g_server);
	g_server = new TankServer();
	if( !g_server->init(&gi) )
	{
		SAFE_DELETE(g_server);
		MessageBoxT(g_env.hMainWnd, "Не удалось запустить сервер. "
			"Проверьте конфигурацию сети", MB_OK|MB_ICONHAND);
		return;
	}

	g_conf.sv_latency->SetInt(gi.latency);

	(new ConnectDlg(GetParent(), "localhost"))->eventClose.bind(&CreateServerDlg::OnCloseChild, this);
	Show(false);
}

void CreateServerDlg::OnCancel()
{
	Close(_resultCancel);
}

void CreateServerDlg::OnCloseChild(int result)
{
	if( _resultCancel == result )
	{
		Show(true);
	}

	if( _resultOK == result )
	{
		Close(_resultOK);
	}
}


///////////////////////////////////////////////////////////////////////////////

ConnectDlg::ConnectDlg(Window *parent, const char *autoConnect)
  : Dialog(parent, 512, 384)
{
	PauseGame(true);

	_auto = (NULL != autoConnect);

	Text *title = new Text(this, GetWidth() / 2, 16, "Соединение с сервером", alignTextCT);
	title->SetTexture("font_default");
	title->Resize(title->GetTextureWidth(), title->GetTextureHeight());


	new Text(this, 20, 65, "Адрес сервера", alignTextLT);
	_name = new Edit(this, 25, 80, 300);
	_name->SetText(g_conf.cl_server->Get());


	new Text(this, 20, 105, "Статус", alignTextLT);
	_status = new List(this, 25, 120, 400, 180);


	_btnOK = new Button(this, 312, 350, "Соединение");
	_btnOK->eventClick.bind(&ConnectDlg::OnOK, this);

	(new Button(this, 412, 350, "Отмена"))->eventClick.bind(&ConnectDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_name);


	if( _auto )
	{
		_ASSERT(g_server);
		_ASSERT(!g_level);

		_name->SetText(autoConnect);
		OnOK();
	}
}

ConnectDlg::~ConnectDlg()
{
	PauseGame(false);
}

void ConnectDlg::OnOK()
{
	_status->DeleteAllItems();

	_btnOK->Enable(false);
	_name->Enable(false);

	if( g_level )
	{
		script_exec(g_env.L, "reset()");
	}
	_ASSERT(NULL == g_level);
	_ASSERT(NULL == g_client);
	g_client = new TankClient();

	if( !g_client->Connect(_name->GetText().c_str(), g_env.hMainWnd) )
	{
		Error("Ошибка сети!");
	}
	else
	{
		SetTimeStep(true);
	}
}

void ConnectDlg::OnCancel()
{
	if( g_server )
	{
		SAFE_DELETE(g_level);
		SAFE_DELETE(g_client);
		SAFE_DELETE(g_server);
	}

	Close(_resultCancel);
}

void ConnectDlg::OnTimeStep(float dt)
{
	DataBlock db;
	while( g_client->GetData(db) )
	{
		switch( db.type() )
		{
			case DBTYPE_GAMEINFO:
			{
				GameInfo &gi = db.cast<GameInfo>();

				if( VERSION != gi.dwVersion )
				{
					Error("Несовместимая версия сервера");
					break;
				}

				g_conf.sv_timelimit->SetInt(gi.timelimit);
				g_conf.sv_fraglimit->SetInt(gi.fraglimit);
				g_conf.sv_fps->SetInt(gi.server_fps);
				g_conf.sv_nightmode->Set(gi.nightmode);

				char msg[MAX_PATH + 32];
				sprintf(msg, "Загрузка карты '%s'...", gi.cMapName);
				_status->AddItem(msg);

				char path[MAX_PATH];
				wsprintf(path, "%s\\%s.map", DIR_MAPS, gi.cMapName);

				if( CalcCRC32(path) != gi.dwMapCRC32 )
				{
					Error("Несовместимая версия карты");
					break;
				}


				SAFE_DELETE(g_level);
				g_level = new Level();

				if( g_level->init_newdm(path, gi.seed) )
				{
					g_conf.cl_map->Set(gi.cMapName);
					g_conf.ui_showmsg->Set(true);
					if( !_auto )
					{
						g_conf.cl_server->Set(_name->GetText().c_str());
					}
				}
				else
				{
					Error("Не удалось загрузить карту");
					break;
				}

				(new WaitingForPlayersDlg(GetParent()))->eventClose = eventClose;
				Close(-1); // close with any code except ok and cancel
				return;
			}

			case DBTYPE_ERRORMSG:
				Error((const char *) db.data());
				break;

			case DBTYPE_TEXTMESSAGE:
				_status->AddItem((const char *) db.data());
				break;
			default:
				_ASSERT(FALSE);
		}

		if( !g_client ) break;
	}
}

void ConnectDlg::Error(const char *msg)
{
	_status->AddItem(msg);
	SAFE_DELETE(g_level);
	SAFE_DELETE(g_client);
	if( g_server )
	{
		SAFE_DELETE(g_server);
	}
	else
	{
		_btnOK->Enable(true);
		_name->Enable(true);
	}
	SetTimeStep(false);
}

///////////////////////////////////////////////////////////////////////////////

WaitingForPlayersDlg::WaitingForPlayersDlg(Window *parent)
  : Dialog(parent, 700, 512)
{
	Text *title = new Text(this, GetWidth() / 2, 16, "Ожидание других игроков", alignTextCT);
	title->SetTexture("font_default");
	title->Resize(title->GetTextureWidth(), title->GetTextureHeight());

	new Text(this, 20, 50, "Кто в игре", alignTextLT);
	_players = new List(this, 20, 65, 512, 200);
	_players->SetTabPos(1, 200);
	_players->SetTabPos(2, 300);
	_players->SetTabPos(3, 400);


	new Text(this, 20, 285, "Окно чата", alignTextLT);
	_buf = new ConsoleBuffer(80, 500, "chat.txt");
	_chat = new Console(this, 20, 300, 512, 200, _buf);
	_chat->SetTexture("ctrl_list");
	_chat->SetEcho(false);
	_chat->eventOnSendCommand.bind(&WaitingForPlayersDlg::OnSendMessage, this);


	_btnOK = new Button(this, 590, 450, "Я готов!");
	_btnOK->eventClick.bind(&WaitingForPlayersDlg::OnOK, this);
	_btnOK->Enable(false);

	(new Button(this, 590, 480, "Отмена"))->eventClick.bind(&WaitingForPlayersDlg::OnCancel, this);


	PlayerDesc pd;
	strcpy(pd.nick, g_conf.cl_playerinfo->GetStr("nick", "Unnamed Player")->Get());
	strcpy(pd.cls, g_conf.cl_playerinfo->GetStr("class", "default")->Get());
	pd.score = 0;
	strcpy(pd.skin, g_conf.cl_playerinfo->GetStr("skin", "red")->Get());
	pd.team = g_conf.cl_playerinfo->GetNum("team", 0)->GetInt();
	g_client->SendDataToServer(DataWrap(pd, DBTYPE_PLAYERINFO));

	SetTimeStep(true);
}

WaitingForPlayersDlg::~WaitingForPlayersDlg()
{
	SAFE_DELETE(_buf);
}

void WaitingForPlayersDlg::OnOK()
{
	DataBlock db(sizeof(dbPlayerReady));
	db.type() = DBTYPE_PLAYERREADY;
	db.cast<dbPlayerReady>().player_id = g_client->GetId();
	db.cast<dbPlayerReady>().ready = TRUE;
	g_client->SendDataToServer(db);
}

void WaitingForPlayersDlg::OnCancel()
{
	SAFE_DELETE(g_level);
	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);

	Close(_resultCancel);
}

void WaitingForPlayersDlg::OnSendMessage(const char *msg)
{
	if( size_t l = strlen(msg) )
	{
		if( 0 == strcmp(msg, "/ping") )
		{
			DataBlock db(sizeof(DWORD));
			db.type() = DBTYPE_PING;
			db.cast<DWORD>() = timeGetTime();
			g_client->SendDataToServer(db);
		}
		else
		{
			DataBlock db(l + 1);
			strcpy((char*) db.data(), msg);
			db.type() = DBTYPE_TEXTMESSAGE;
			g_client->SendDataToServer(db);
		}
	}
}

void WaitingForPlayersDlg::OnTimeStep(float dt)
{
	DataBlock db;
	while( g_client->GetData(db) )
	{
		switch( db.type() )
		{
		case DBTYPE_PING:
			_buf->printf("%d ms\n", timeGetTime() - db.cast<DWORD>());
			break;
		case DBTYPE_PLAYERREADY:
		{
			int count = g_level->GetList(LIST_players).size();
			_ASSERT(_players->GetSize() == count);

			const DWORD who = db.cast<dbPlayerReady>().player_id;

			int index = 0;
			for( ;index < count; ++index )
			{
				GC_Player *player = (GC_Player *) _players->GetItemData(index);
				_ASSERT(player);
				_ASSERT(!player->IsKilled());
				_ASSERT(0 != player->GetNetworkID());

				if( who == player->GetNetworkID() )
				{
					if( db.cast<dbPlayerReady>().ready )
					{
						_players->SetItemText(index, 3, "Готов");
					}
					else
					{
						_players->SetItemText(index, 3, "");
					}
					break;
				}
			}
			_ASSERT(index < count);
			break;
		}

		case DBTYPE_PLAYERQUIT:
		{
			int count = g_level->GetList(LIST_players).size();
			_ASSERT(_players->GetSize() == count);

			const DWORD who = db.cast<DWORD>();

			int index = 0;
			for( ; index < count; ++index )
			{
				GC_Player *player = (GC_Player *) _players->GetItemData(index);
				_ASSERT(player);
				_ASSERT(!player->IsKilled());
				_ASSERT(0 != player->GetNetworkID());

				if( who == player->GetNetworkID() )
				{
					_players->DeleteItem(index);
					_buf->printf("%s покинул игру.\n", player->GetNick());
					player->Kill();
					break;
				}
			}
			_ASSERT(index < count);
			break;
		}

		case DBTYPE_NEWPLAYER:
		{
			PlayerDescEx &pd = db.cast<PlayerDescEx>();

			GC_Player *player = NULL;
			if( g_client->GetId() == pd.id )
			{
				player = new GC_PlayerLocal();
				static_cast<GC_PlayerLocal *>(player)
					->SetProfile(g_conf.cl_playerinfo->GetStr("profile")->Get());
			}
			else
			{
				player = new GC_PlayerRemote(pd.id);
			}
			player->SetClass(pd.cls);
			player->SetNick(pd.nick);
			player->SetSkin(pd.skin);
			player->SetTeam(pd.team);
			player->UpdateSkin();

			int index = _players->AddItem(player->GetNick().c_str(), (UINT_PTR) player);
			_players->SetItemText(index, 1, player->GetSkin().c_str());

			std::ostringstream tmp;
			tmp << "команда " << player->GetTeam();
				_players->SetItemText(index, 2, tmp.str().c_str());

		//	if( pd.type >= MAX_HUMANS )
		//		ListView_SetItemText(hwndLV, ListView_GetItemCount(hwndLV) - 1, 3, "Бот");

			_buf->printf("%s вошел в игру.\n", player->GetNick().c_str());

			_btnOK->Enable(true);
			break;
		}

		case DBTYPE_ERRORMSG:
			_btnOK->Enable(false);
		case DBTYPE_TEXTMESSAGE:
			_buf->printf("%s\n", (const char *) db.data());
			break;

		case DBTYPE_STARTGAME:
		{
			for( size_t i = 0; i < g_client->_latency; ++i )
			{
				g_client->SendControl(ControlPacket());
			}
			Close(_resultOK);
			break;
		}
		default:
			_ASSERT(FALSE);
		} // end of switch( db.type() )
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
