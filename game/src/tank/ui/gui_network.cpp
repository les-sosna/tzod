// gui_network.cpp

#include "stdafx.h"

#include "gui.h"
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
#include "config/Language.h"

#include "core/Console.h"

#include "network/TankServer.h"
#include "network/TankClient.h"
#include "network/HttpClient.h"

#include "gc/Player.h"
#include "gc/ai.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

CreateServerDlg::CreateServerDlg(Window *parent)
  : Dialog(parent, 770, 450)
{
	Text *title = new Text(this, GetWidth() / 2, 16, g_lang->net_server_title->Get(), alignTextCT);
	title->SetTexture("font_default");
	title->Resize(title->GetTextureWidth(), title->GetTextureHeight());

	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 20;
	float x4 = x3 + 20;

	//
	// map list
	//

	new Text(this, x1, 46, g_lang->choose_map->Get(), alignTextLT);

	_maps = new MapList(this, x1, 62, x2 - x1, 300);
	GetManager()->SetFocusWnd(_maps);


	//
	// settings
	//

	{
		float y =  56;

		_nightMode = new CheckBox(this, x3, y, g_lang->night_mode->Get());
		_nightMode->SetCheck( g_conf->cl_nightmode->Get() );


		new Text(this, x3, y+=30, g_lang->game_speed->Get(), alignTextLT);
		_gameSpeed = new Edit(this, x4, y+=15, 80);
		_gameSpeed->SetInt(g_conf->cl_speed->GetInt());

		new Text(this, x3, y+=30, g_lang->frag_limit->Get(), alignTextLT);
		_fragLimit = new Edit(this, x4, y+=15, 80);
		_fragLimit->SetInt(g_conf->cl_fraglimit->GetInt());

		new Text(this, x3, y+=30, g_lang->time_limit->Get(), alignTextLT);
		_timeLimit = new Edit(this, x4, y+=15, 80);
		_timeLimit->SetInt(g_conf->cl_timelimit->GetInt());

		new Text(this, x3+30, y+=30, g_lang->zero_no_limits->Get(), alignTextLT);

		new Text(this, x3, y+=40, g_lang->net_server_fps->Get(), alignTextLT);
		_svFps = new Edit(this, x4, y+=15, 100);
		_svFps->SetInt(g_conf->sv_fps->GetInt());

//		new Text(this, x3, y+=30, g_lang->net_server_latency->Get(), alignTextLT);
//		_svLatency = new Edit(this, x4, y+=15, 100);
//		_svLatency->SetInt(g_conf->sv_latency->GetInt());
	}

	Button *btn;
	btn = new Button(this, 544, 410, g_lang->net_server_ok->Get());
	btn->eventClick.bind(&CreateServerDlg::OnOK, this);

	btn = new Button(this, 656, 410, g_lang->net_server_cancel->Get());
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
//	gi.latency    = __max(MIN_LATENCY, __min(MAX_LATENCY, _svLatency->GetInt()));
	gi.nightmode  = _nightMode->GetCheck();

	strcpy(gi.cMapName, fn.c_str());
	strcpy(gi.cServerName, "ZOD Server");

	_ASSERT(NULL == g_server);
	g_server = new TankServer();
	if( !g_server->init(&gi) )
	{
		SAFE_DELETE(g_server);
		MessageBoxT(g_env.hMainWnd, g_lang->net_server_error->Get().c_str(), MB_OK|MB_ICONHAND);
		return;
	}

	g_conf->sv_latency->SetInt(1);

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
  , _auto(NULL != autoConnect)
{
	PauseGame(true);

	Text *title = new Text(this, GetWidth() / 2, 16, g_lang->net_connect_title->Get(), alignTextCT);
	title->SetTexture("font_default");
	title->Resize(title->GetTextureWidth(), title->GetTextureHeight());


	new Text(this, 20, 65, g_lang->net_connect_address->Get(), alignTextLT);
	_name = new Edit(this, 25, 80, 300);
	_name->SetText(g_conf->cl_server->Get());


	new Text(this, 20, 105, g_lang->net_connect_status->Get(), alignTextLT);
	_status = new List(this, 25, 120, 400, 180);


	_btnOK = new Button(this, 312, 350, g_lang->net_connect_ok->Get());
	_btnOK->eventClick.bind(&ConnectDlg::OnOK, this);

	(new Button(this, 412, 350, g_lang->net_connect_cancel->Get()))
		->eventClick.bind(&ConnectDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_name);


	if( _auto )
	{
		_ASSERT(g_server);
		_ASSERT(g_level->IsEmpty());

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

	if( !_auto )
		script_exec(g_env.L, "reset()");

	_ASSERT(g_level->IsEmpty());
	_ASSERT(NULL == g_client);
	g_client = new TankClient();
	g_client->eventNewData.bind(&ConnectDlg::OnNewData, this);
	g_client->Connect(_name->GetText());
}

void ConnectDlg::OnCancel()
{
	if( g_server )
	{
		g_level->Clear();
		SAFE_DELETE(g_client);
		SAFE_DELETE(g_server);
	}

	Close(_resultCancel);
}

void ConnectDlg::OnNewData(const DataBlock &db)
{
	switch( db.type() )
	{
		case DBTYPE_GAMEINFO:
		{
			GameInfo &gi = db.cast<GameInfo>();

			if( VERSION != gi.dwVersion )
			{
				Error(g_lang->net_connect_error_server_version->Get().c_str());
				break;
			}

			g_conf->sv_timelimit->SetInt(gi.timelimit);
			g_conf->sv_fraglimit->SetInt(gi.fraglimit);
			g_conf->sv_fps->SetInt(gi.server_fps);
			g_conf->sv_nightmode->Set(gi.nightmode);

			char msg[MAX_PATH + 32];
			sprintf(msg, g_lang->net_connect_loading_map_x->Get().c_str(), gi.cMapName);
			_status->AddItem(msg);

			char path[MAX_PATH];
			wsprintf(path, "%s\\%s.map", DIR_MAPS, gi.cMapName);

			if( CalcCRC32(path) != gi.dwMapCRC32 )
			{
				Error(g_lang->net_connect_error_map_version->Get().c_str());
				break;
			}


			g_level->Clear();
			if( !g_level->init_newdm(path, gi.seed) )
			{
				Error(g_lang->net_connect_error_map->Get().c_str());
				break;
			}
			g_level->PauseLocal(true); // paused until game is started

			g_conf->cl_map->Set(gi.cMapName);
			g_conf->ui_showmsg->Set(true);
			if( !_auto )
			{
				g_conf->cl_server->Set(_name->GetText());
			}

			(new WaitingForPlayersDlg(GetParent()))->eventClose = eventClose;
			Close(-1); // close with any code except ok and cancel
			return;
		}

		case DBTYPE_ERRORMSG:
			Error((const char *) db.Data());
			break;

		case DBTYPE_TEXTMESSAGE:
			_status->AddItem((const char *) db.Data());
			break;
		default:
			_ASSERT(FALSE);
	}
}

void ConnectDlg::Error(const char *msg)
{
	_status->AddItem(msg);
	g_level->Clear();
	if( !g_server )
	{
		_btnOK->Enable(true);
		_name->Enable(true);
	}
	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);
	SetTimeStep(false);
}

///////////////////////////////////////////////////////////////////////////////

InternetDlg::InternetDlg(Window *parent)
  : Dialog(parent, 512, 384)
  , _client(new HttpClient())
{
	_client->eventResult.bind(&InternetDlg::OnResult, this);
	PauseGame(true);

	Text *title = new Text(this, GetWidth() / 2, 16, g_lang->net_internet_title->Get(), alignTextCT);
	title->SetTexture("font_default");
	title->Resize(title->GetTextureWidth(), title->GetTextureHeight());


	new Text(this, 20, 65, g_lang->net_connect_address->Get(), alignTextLT);
	_name = new Edit(this, 25, 80, 300);
	_name->SetText("tzod.fatal.ru/lobby/");


	new Text(this, 20, 105, g_lang->net_connect_status->Get(), alignTextLT);
	_status = new List(this, 25, 120, 400, 180);


	_btnOK = new Button(this, 312, 350, g_lang->net_connect_ok->Get());
	_btnOK->eventClick.bind(&InternetDlg::OnOK, this);

	(new Button(this, 412, 350, g_lang->net_connect_cancel->Get()))
		->eventClick.bind(&InternetDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_name);
}

InternetDlg::~InternetDlg()
{
	PauseGame(false);
}

void InternetDlg::OnOK()
{
	_status->DeleteAllItems();

	_btnOK->Enable(false);
	_name->Enable(false);

	HttpClient::Param param;
	param["ver"] = "149b";
	_client->Get(_name->GetText(), param);
}

void InternetDlg::OnCancel()
{
	Close(_resultCancel);
}

void InternetDlg::OnResult(int err, const std::string &result, const HttpParam *headers)
{
	_status->AddItem(result);
}

void InternetDlg::Error(const char *msg)
{
	_status->AddItem(msg);
	_btnOK->Enable(true);
	_name->Enable(true);
}

///////////////////////////////////////////////////////////////////////////////

WaitingForPlayersDlg::WaitingForPlayersDlg(Window *parent)
  : Dialog(parent, 680, 512)
  , _buf(new ConsoleBuffer(80, 500, "chat.txt"))
{
	_ASSERT(g_client);
	g_client->eventNewData.bind(&WaitingForPlayersDlg::OnNewData, this);

	//
	// create controls
	//

	Text *title = new Text(this, GetWidth() / 2, 16, g_lang->net_chatroom_title->Get(), alignTextCT);
	title->SetTexture("font_default");
	title->Resize(title->GetTextureWidth(), title->GetTextureHeight());

	new Text(this, 20, 50, g_lang->net_chatroom_players->Get(), alignTextLT);
	_players = new List(this, 20, 65, 512, 70);
	_players->SetTabPos(1, 200);
	_players->SetTabPos(2, 300);
	_players->SetTabPos(3, 400);

	new Text(this, 20, 150, g_lang->net_chatroom_bots->Get(), alignTextLT);
	_bots = new List(this, 20, 165, 512, 100);
	_bots->SetTabPos(1, 200);
	_bots->SetTabPos(2, 300);
	_bots->SetTabPos(3, 400);

	(new Button(this, 560, 180, g_lang->net_chatroom_bot_new->Get()))->eventClick.bind(&WaitingForPlayersDlg::OnAddBot, this);


	new Text(this, 20, 285, g_lang->net_chatroom_chat_window->Get(), alignTextLT);
	_chat = new Console(this, 20, 300, 512, 200, GetRawPtr(_buf));
	_chat->SetTexture("ctrl_list");
	_chat->SetEcho(false);
	_chat->eventOnSendCommand.bind(&WaitingForPlayersDlg::OnSendMessage, this);


	_btnOK = new Button(this, 560, 450, g_lang->net_chatroom_ready_button->Get());
	_btnOK->eventClick.bind(&WaitingForPlayersDlg::OnOK, this);
	_btnOK->Enable(false);

	(new Button(this, 560, 480, g_lang->common_cancel->Get()))->eventClick.bind(&WaitingForPlayersDlg::OnCancel, this);


	//
	// send player info
	//

	PlayerDesc pd;
	strcpy(pd.nick, g_conf->cl_playerinfo->GetStr("nick", "Unnamed Player")->Get().c_str());
	strcpy(pd.cls, g_conf->cl_playerinfo->GetStr("class", "default")->Get().c_str());
	pd.score = 0;
	strcpy(pd.skin, g_conf->cl_playerinfo->GetStr("skin", "red")->Get().c_str());
	pd.team = g_conf->cl_playerinfo->GetNum("team", 0)->GetInt();
	g_client->SendDataToServer(DataWrap(pd, DBTYPE_PLAYERINFO));

	// send ping request
	DWORD t = timeGetTime();
	g_client->SendDataToServer(DataWrap(t, DBTYPE_PING));
}

WaitingForPlayersDlg::~WaitingForPlayersDlg()
{
}

void WaitingForPlayersDlg::OnAddBot()
{
	(new EditBotDlg(this, g_conf->ui_netbotinfo))->eventClose.bind(&WaitingForPlayersDlg::OnAddBotClose, this);
}

void WaitingForPlayersDlg::OnAddBotClose(int result)
{
	if( _resultOK == result )
	{
		BotDesc bd;
		strcpy(bd.nick, g_conf->ui_netbotinfo->GetStr("nick", "Bot")->Get().c_str());
		strcpy(bd.cls, g_conf->ui_netbotinfo->GetStr("class", "default")->Get().c_str());
		strcpy(bd.skin, g_conf->ui_netbotinfo->GetStr("skin", "red")->Get().c_str());
		bd.score = 0;
		bd.team = g_conf->ui_netbotinfo->GetNum("team", 0)->GetInt();
		bd.level = g_conf->ui_netbotinfo->GetNum("level", 2)->GetInt();

		g_client->SendDataToServer(DataWrap(bd, DBTYPE_NEWBOT));
	}
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
	g_level->Clear();
	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);

	Close(_resultCancel);
}

void WaitingForPlayersDlg::OnSendMessage(const char *msg)
{
	if( msg[0] )
	{
		if( 0 == strcmp(msg, "/ping") )
		{
			DWORD t = timeGetTime();
			g_client->SendDataToServer(DataWrap(t, DBTYPE_PING));
		}
		else
		{
			g_client->SendDataToServer(DataWrap(std::string(msg), DBTYPE_TEXTMESSAGE));
		}
	}
}

void WaitingForPlayersDlg::OnNewData(const DataBlock &db)
{
	switch( db.type() )
	{
	case DBTYPE_PING:
	{
		DWORD ping = timeGetTime() - db.cast<DWORD>();
		if( _pings.size() < _maxPings )
		{
			_pings.push_back(ping);
			if( _pings.size() == _maxPings )
			{
				DWORD avg = 0;
				for( size_t i = 0; i < _pings.size(); ++i )
				{
					avg += _pings[i];
				}
				avg /= _pings.size();
				_buf->printf("ping %d ms\n", avg);
				_buf->printf("latency %d frames\n", g_conf->sv_fps->GetInt() * avg / 1000);
			}
			else
			{
				DWORD t = timeGetTime();
				g_client->SendDataToServer(DataWrap(t, DBTYPE_PING));
			}
		}
		else
		{
			_buf->printf("%d ms\n", ping);
		}
		break;
	}
	case DBTYPE_PLAYERREADY:
	{
		int count = g_level->GetList(LIST_players).size();
		_ASSERT(_players->GetItemCount() <= count); // count includes bots

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
					_players->SetItemText(index, 3, g_lang->net_chatroom_player_ready->Get());
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
		_ASSERT(_players->GetItemCount() == count);

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
				_buf->printf(g_lang->net_chatroom_player_x_disconnected->Get().c_str(), player->GetNick().c_str());
				_buf->printf("\n");
				player->Kill();
				break;
			}
		}
		_ASSERT(index < count);
		break;
	}

	case DBTYPE_NEWBOT:
	{
		BotDesc &bd = db.cast<BotDesc>();
		GC_PlayerAI *ai = new GC_PlayerAI();

		ai->SetClass(bd.cls);
		ai->SetNick(bd.nick);
		ai->SetSkin(bd.skin);
		ai->SetTeam(bd.team);
		ai->SetLevel(__max(0, __min(AI_MAX_LEVEL, bd.level)));
		ai->UpdateSkin();

		// nick & skin
		int index = _bots->AddItem(ai->GetNick(), (UINT_PTR) ai);
		_bots->SetItemText(index, 1, ai->GetSkin());

		// team
		std::ostringstream tmp;
		tmp << g_lang->net_chatroom_team->Get() << ai->GetTeam();
		_bots->SetItemText(index, 2, tmp.str());

		// level
		_bots->SetItemText(index, 3, EditBotDlg::levels[ai->GetLevel()]);

		break;
	}

	case DBTYPE_NEWPLAYER:
	{
		PlayerDescEx &pd = db.cast<PlayerDescEx>();

		GC_Player *player = NULL;
		if( g_client->GetId() == pd.id )
		{
			player = new GC_PlayerLocal();
			const string_t &profile = g_conf->cl_playerinfo->GetStr("profile")->Get();
			if( profile.empty() )
			{
				static_cast<GC_PlayerLocal *>(player)->SelectFreeProfile();
			}
			else
			{
				static_cast<GC_PlayerLocal *>(player)->SetProfile(profile);
			}
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

		int index = _players->AddItem(player->GetNick(), (UINT_PTR) player);
		_players->SetItemText(index, 1, player->GetSkin());

		std::ostringstream tmp;
		tmp << g_lang->net_chatroom_team->Get() << player->GetTeam();
		_players->SetItemText(index, 2, tmp.str());

		_buf->printf(g_lang->net_chatroom_player_x_connected->Get().c_str(), player->GetNick().c_str());
		_buf->printf("\n");

		_btnOK->Enable(true);
		break;
	}

	case DBTYPE_ERRORMSG:
		_btnOK->Enable(false);
	case DBTYPE_TEXTMESSAGE:
		_buf->printf("%s\n", (const char *) db.Data());
		break;

	case DBTYPE_STARTGAME:
		g_client->SendControl(ControlPacket()); // initial empty packet
		g_client->eventNewData.bind(&Level::OnNewData, GetRawPtr(g_level));
		g_level->PauseLocal(false);

		g_level->_dropedFrames = 0; // FIXME
		if( g_conf->sv_autoLatency->Get() )
		{
			DWORD avg = 0;
			for( size_t i = 0; i < _pings.size(); ++i )
			{
				avg += _pings[i];
			}
			avg /= _pings.size();
			// set initial latency for auto adjustment algorithm
			g_conf->sv_latency->SetInt(__max(1, g_conf->sv_fps->GetInt() * avg / 1000));
		}

		Close(_resultOK);
		break;

	default:
		_ASSERT(FALSE);
	} // end of switch( db.type() )
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
