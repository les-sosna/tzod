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
#include "ConsoleBuffer.h"
#include "Combo.h"

#include "Interface.h"
#include "functions.h"
#include "Level.h"
#include "Macros.h"
#include "script.h"

#include "config/Config.h"
#include "config/Language.h"

#include "core/debug.h"

#include "network/TankServer.h"
#include "network/TankClient.h"
#include "network/LobbyClient.h"

#include "gc/Player.h"
#include "gc/ai.h"

#include "fs/FileSystem.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

CreateServerDlg::CreateServerDlg(Window *parent)
  : Dialog(parent, 770, 450)
{
	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang->net_server_title->Get(), alignTextCT);
	title->SetFont("font_default");

	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 20;
	float x4 = x3 + 20;

	//
	// map list
	//

	Text::Create(this, x1, 46, g_lang->choose_map->Get(), alignTextLT);

	_maps = new MapList(this, x1, 62, x2 - x1, 300);
	GetManager()->SetFocusWnd(_maps);


	//
	// settings
	//

	{
		float y =  56;

		_nightMode = CheckBox::Create(this, x3, y, g_lang->night_mode->Get());
		_nightMode->SetCheck( g_conf->cl_nightmode->Get() );


		Text::Create(this, x3, y+=30, g_lang->game_speed->Get(), alignTextLT);
		_gameSpeed = Edit::Create(this, x4, y+=15, 80);
		_gameSpeed->SetInt(g_conf->cl_speed->GetInt());

		Text::Create(this, x3, y+=30, g_lang->frag_limit->Get(), alignTextLT);
		_fragLimit = Edit::Create(this, x4, y+=15, 80);
		_fragLimit->SetInt(g_conf->cl_fraglimit->GetInt());

		Text::Create(this, x3, y+=30, g_lang->time_limit->Get(), alignTextLT);
		_timeLimit = Edit::Create(this, x4, y+=15, 80);
		_timeLimit->SetInt(g_conf->cl_timelimit->GetInt());

		Text::Create(this, x3+30, y+=30, g_lang->zero_no_limits->Get(), alignTextLT);

		Text::Create(this, x3, y+=40, g_lang->net_server_fps->Get(), alignTextLT);
		_svFps = Edit::Create(this, x4, y+=15, 100);
		_svFps->SetInt(g_conf->sv_fps->GetInt());
	}


	//
	// Lobby
	//
	{
		_lobbyEnable = CheckBox::Create(this, 32, 390, g_lang->net_server_use_lobby->Get());
		_lobbyList = ComboBox::Create(this, 32, 415, 200);
		_lobbyAdd = Button::Create(this, g_lang->net_server_add_lobby->Get(), 250, 410);

		_lobbyEnable->SetCheck(g_conf->sv_use_lobby->Get());
		_lobbyList->SetEnabled(_lobbyEnable->GetCheck());
		_lobbyAdd->SetEnabled(_lobbyEnable->GetCheck());
		for( size_t i = 0; i < g_conf->lobby_servers->GetSize(); ++i )
		{
			_lobbyList->GetList()->AddItem(g_conf->lobby_servers->GetStr(i)->Get());
			if( !i || g_conf->sv_lobby->Get() == g_conf->lobby_servers->GetStr(i)->Get() )
			{
				_lobbyList->SetCurSel(i);
			}
		}
		_lobbyList->GetList()->AlignHeightToContent(128);

		_lobbyEnable->eventClick.bind(&CreateServerDlg::OnLobbyEnable, this);
	}

	Button *btn;
	btn = Button::Create(this, g_lang->net_server_ok->Get(), 544, 410);
	btn->eventClick.bind(&CreateServerDlg::OnOK, this);

	btn = Button::Create(this, g_lang->net_server_cancel->Get(), 656, 410);
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

	SafePtr<LobbyClient> announcer;
	g_conf->sv_use_lobby->Set(_lobbyEnable->GetCheck());
	if( _lobbyEnable->GetCheck() )
	{
		if( -1 == _lobbyList->GetCurSel() )
		{
			return;
		}
		g_conf->sv_lobby->Set(_lobbyList->GetList()->GetItemText(_lobbyList->GetCurSel()));
		announcer = WrapRawPtr(new LobbyClient());
		announcer->SetLobbyUrl(g_conf->sv_lobby->Get());
	}

	script_exec(g_env.L, "reset()");


	string_t path = DIR_MAPS;
	path += "\\";
	path += fn + ".map";

	GameInfo gi = {0};
	memcpy(gi.exeVer, g_md5.bytes, 16);
	gi.seed       = rand();
	gi.fraglimit  = __max(0, __min(MAX_FRAGLIMIT, _fragLimit->GetInt()));
	gi.timelimit  = __max(0, __min(MAX_TIMELIMIT, _timeLimit->GetInt()));
	gi.server_fps = __max(MIN_NETWORKSPEED, __min(MAX_NETWORKSPEED, _svFps->GetInt()));
	gi.nightmode  = _nightMode->GetCheck();

	strcpy(gi.cMapName, fn.c_str());
	strcpy(gi.cServerName, "ZOD Server");

	assert(NULL == g_server);
	try
	{
		SafePtr<FS::MemMap> m = g_fs->Open(path)->QueryMap();
		MD5_CTX md5;
		MD5Init(&md5);
		MD5Update(&md5, m->GetData(), m->GetSize());
		MD5Final(&md5);
		memcpy(gi.mapVer, md5.digest, 16);

		g_server = new TankServer(gi, announcer);
	}
	catch( const std::exception &e )
	{
		TRACE("%s", e.what());
		MessageBox(g_env.hMainWnd, g_lang->net_server_error->Get().c_str(), TXT_VERSION, MB_OK|MB_ICONERROR);
		return;
	}

//	g_conf->sv_latency->SetInt(1);

	(new ConnectDlg(GetParent(), "localhost"))->eventClose.bind(&CreateServerDlg::OnCloseChild, this);
	SetVisible(false);
}

void CreateServerDlg::OnCancel()
{
	Close(_resultCancel);
}

void CreateServerDlg::OnLobbyEnable()
{
	_lobbyList->SetEnabled(_lobbyEnable->GetCheck());
	_lobbyAdd->SetEnabled(_lobbyEnable->GetCheck());
}

void CreateServerDlg::OnCloseChild(int result)
{
	if( _resultCancel == result )
	{
		SetVisible(true);
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

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang->net_connect_title->Get(), alignTextCT);
	title->SetFont("font_default");


	Text::Create(this, 20, 65, g_lang->net_connect_address->Get(), alignTextLT);
	_name = Edit::Create(this, 25, 80, 300);
	_name->SetText(g_conf->cl_server->Get());


	Text::Create(this, 20, 105, g_lang->net_connect_status->Get(), alignTextLT);
	_status = List::Create(this, 25, 120, 400, 180);


	_btnOK = Button::Create(this, g_lang->net_connect_ok->Get(), 312, 350);
	_btnOK->eventClick.bind(&ConnectDlg::OnOK, this);

	Button::Create(this, g_lang->net_connect_cancel->Get(), 412, 350)->eventClick.bind(&ConnectDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_name);


	if( _auto )
	{
		assert(g_level->IsEmpty());
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

	_btnOK->SetEnabled(false);
	_name->SetEnabled(false);

	if( !_auto )
		script_exec(g_env.L, "reset()");

	assert(g_level->IsEmpty());
	assert(NULL == g_client);
	g_client = new TankClient();
	g_client->eventConnected.bind(&ConnectDlg::OnConnected, this);
	g_client->eventErrorMessage.bind(&ConnectDlg::OnError, this);
	g_client->eventTextMessage.bind(&ConnectDlg::OnMessage, this);
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

void ConnectDlg::OnConnected()
{
	if( !_auto )
	{
		g_conf->cl_server->Set(_name->GetText());
	}
	(new WaitingForPlayersDlg(GetParent()))->eventClose = eventClose;
	Close(-1); // close with any code except ok and cancel
}

void ConnectDlg::OnError(const std::string &msg)
{
	_status->AddItem(msg);
	g_level->Clear();
	if( !g_server )
	{
		_btnOK->SetEnabled(true);
		_name->SetEnabled(true);
	}
	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);
}

void ConnectDlg::OnMessage(const std::string &msg)
{
	_status->AddItem(msg);
}

///////////////////////////////////////////////////////////////////////////////

InternetDlg::InternetDlg(Window *parent)
  : Dialog(parent, 450, 384)
  , _client(new LobbyClient())
{
	_client->eventError.bind(&InternetDlg::OnLobbyError, this);
	_client->eventServerListReply.bind(&InternetDlg::OnLobbyList, this);

	PauseGame(true);

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang->net_internet_title->Get(), alignTextCT);
	title->SetFont("font_default");


	Text::Create(this, 20, 65, g_lang->net_internet_lobby_address->Get(), alignTextLT);
	_name = Edit::Create(this, 25, 80, 300);
	_name->SetText("tzod.fatal.ru/lobby/");


	Text::Create(this, 20, 105, g_lang->net_internet_server_list->Get(), alignTextLT);
	_servers = List::Create(this, 25, 120, 400, 180);
	_servers->eventChangeCurSel.bind(&InternetDlg::OnSelectServer, this);
	_status = Text::Create(_servers, _servers->GetWidth() / 2, _servers->GetHeight() / 2, "", alignTextCC);
	_status->SetFontColor(0x7f7f7f7f);


	_btnRefresh = Button::Create(this, g_lang->net_internet_refresh->Get(), 25, 320);
	_btnRefresh->eventClick.bind(&InternetDlg::OnRefresh, this);

	_btnConnect = Button::Create(this, g_lang->net_internet_connect->Get(), 175, 320);
	_btnConnect->eventClick.bind(&InternetDlg::OnConnect, this);
	_btnConnect->SetEnabled(false);

	Button::Create(this, g_lang->net_internet_cancel->Get(), 325, 320)->eventClick.bind(&InternetDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_name);

	OnRefresh();
}

InternetDlg::~InternetDlg()
{
	PauseGame(false);
}

void InternetDlg::OnRefresh()
{
	_servers->DeleteAllItems();
	_status->SetText(g_lang->net_internet_searching->Get());

	_btnRefresh->SetEnabled(false);
	_name->SetEnabled(false);

	_client->SetLobbyUrl(_name->GetText());
	_client->RequestServerList();
}

void InternetDlg::OnConnect()
{
	if( -1 != _servers->GetCurSel() )
	{
		const std::string &addr = _servers->GetItemText(_servers->GetCurSel());
		(new ConnectDlg(GetParent(), addr.c_str()))->eventClose.bind(&InternetDlg::OnCloseChild, this);
		SetVisible(false);
	}
}

void InternetDlg::OnCancel()
{
	Close(_resultCancel);
}

void InternetDlg::OnSelectServer(int idx)
{
	_btnConnect->SetEnabled(-1 != idx);
}

void InternetDlg::OnLobbyError(const std::string &msg)
{
	Error(msg.c_str());
}

void InternetDlg::OnLobbyList(const std::vector<std::string> &result)
{
	_status->SetText(result.empty() ? g_lang->net_internet_not_found->Get() : "");
	for( size_t i = 0; i < result.size(); ++i )
	{
		_servers->AddItem(result[i]);
	}
	_btnRefresh->SetEnabled(true);
	_name->SetEnabled(true);
}

void InternetDlg::Error(const char *msg)
{
	_status->SetText(msg);
	_btnRefresh->SetEnabled(true);
	_name->SetEnabled(true);
}

void InternetDlg::OnCloseChild(int result)
{
	if( _resultCancel == result )
	{
		SetVisible(true);
	}

	if( _resultOK == result )
	{
		Close(_resultOK);
	}
}


///////////////////////////////////////////////////////////////////////////////

static PlayerDesc GetPlayerDescFromConf(ConfVarTable *p)
{
	PlayerDesc result;
	result.nick = p->GetStr("nick", "Unnamed Player")->Get();
	result.cls = p->GetStr("class", "default")->Get();
	result.skin = p->GetStr("skin", "red")->Get();
	result.team = p->GetNum("team", 0)->GetInt();
	return result;
}

WaitingForPlayersDlg::WaitingForPlayersDlg(Window *parent)
  : Dialog(parent, 680, 512)
  , _buf(new UI::ConsoleBuffer(80, 500))
  , _players(NULL)
  , _bots(NULL)
  , _chat(NULL)
  , _btnOK(NULL)
  , _btnProfile(NULL)
{
	assert(g_client);
	g_client->eventErrorMessage.bind(&WaitingForPlayersDlg::OnError, this);
	g_client->eventTextMessage.bind(&WaitingForPlayersDlg::OnMessage, this);
	g_client->eventPlayerReady.bind(&WaitingForPlayersDlg::OnPlayerReady, this);
	g_client->eventPlayersUpdate.bind(&WaitingForPlayersDlg::OnPlayersUpdate, this);
	g_client->eventStartGame.bind(&WaitingForPlayersDlg::OnStartGame, this);


	//
	// create controls
	//

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang->net_chatroom_title->Get(), alignTextCT);
	title->SetFont("font_default");

	Text::Create(this, 20, 50, g_lang->net_chatroom_players->Get(), alignTextLT);
	_players = List::Create(this, 20, 65, 512, 70);
	_players->SetTabPos(1, 200);
	_players->SetTabPos(2, 300);
	_players->SetTabPos(3, 400);

	_btnProfile = Button::Create(this, g_lang->net_chatroom_my_profile->Get(), 560, 65);
	_btnProfile->eventClick.bind(&WaitingForPlayersDlg::OnChangeProfileClick, this);

	Text::Create(this, 20, 150, g_lang->net_chatroom_bots->Get(), alignTextLT);
	_bots = List::Create(this, 20, 165, 512, 100);
	_bots->SetTabPos(1, 200);
	_bots->SetTabPos(2, 300);
	_bots->SetTabPos(3, 400);

	Button::Create(this, g_lang->net_chatroom_bot_new->Get(), 560, 180)->eventClick.bind(&WaitingForPlayersDlg::OnAddBotClick, this);


	Text::Create(this, 20, 285, g_lang->net_chatroom_chat_window->Get(), alignTextLT);

	_chat = Console::Create(this, 20, 300, 512, 200, _buf.get());
	_chat->SetTexture("ui/list", false);
	_chat->SetEcho(false);
	_chat->eventOnSendCommand.bind(&WaitingForPlayersDlg::OnSendMessage, this);


	_btnOK = Button::Create(this, g_lang->net_chatroom_ready_button->Get(), 560, 450);
	_btnOK->eventClick.bind(&WaitingForPlayersDlg::OnOK, this);
	_btnOK->SetEnabled(false);

	Button::Create(this, g_lang->common_cancel->Get(), 560, 480)->eventClick.bind(&WaitingForPlayersDlg::OnCancel, this);


	//
	// send player info
	//

	g_client->SendPlayerInfo(GetPlayerDescFromConf(g_conf->cl_playerinfo));

	// send ping request
//	DWORD t = timeGetTime();
//	g_client->SendDataToServer(DataWrap(t, DBTYPE_PING));
}

WaitingForPlayersDlg::~WaitingForPlayersDlg()
{
	if( g_client )
	{
		g_client->eventErrorMessage.clear();
		g_client->eventTextMessage.clear();
		g_client->eventPlayerReady.clear();
		g_client->eventPlayersUpdate.clear();
		g_client->eventStartGame.clear();
	}
}

void WaitingForPlayersDlg::OnCloseProfileDlg(int result)
{
	_btnProfile->SetEnabled(true);
	_btnOK->SetEnabled(true);

	if( _resultOK == result )
	{
		g_client->SendPlayerInfo(GetPlayerDescFromConf(g_conf->cl_playerinfo));
	}
}

void WaitingForPlayersDlg::OnChangeProfileClick()
{
	_btnProfile->SetEnabled(false);
	_btnOK->SetEnabled(false);
	EditPlayerDlg *dlg = new EditPlayerDlg(GetParent(), g_conf->cl_playerinfo);
	dlg->eventClose.bind(&WaitingForPlayersDlg::OnCloseProfileDlg, this);
}

void WaitingForPlayersDlg::OnAddBotClick()
{
	(new EditBotDlg(this, g_conf->ui_netbotinfo))->eventClose.bind(&WaitingForPlayersDlg::OnAddBotClose, this);
}

void WaitingForPlayersDlg::OnAddBotClose(int result)
{
	if( _resultOK == result )
	{
		BotDesc bd;
		bd.pd = GetPlayerDescFromConf(g_conf->ui_netbotinfo);
		bd.level = g_conf->ui_netbotinfo->GetNum("level", 2)->GetInt();
		g_client->SendAddBot(bd);
	}
}

void WaitingForPlayersDlg::OnOK()
{
	g_client->SendPlayerReady(true);
}

void WaitingForPlayersDlg::OnCancel()
{
	g_level->Clear();
	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);

	Close(_resultCancel);
}

void WaitingForPlayersDlg::OnSendMessage(const string_t &msg)
{
	if( !msg.empty() )
	{
//		if( 0 == strcmp(msg, "/ping") )
//		{
//			DWORD t = timeGetTime();
//			g_client->SendDataToServer(DataWrap(t, DBTYPE_PING));
//		}
//		else
		{
			g_client->SendTextMessage(msg);
		}
	}
}

void WaitingForPlayersDlg::OnError(const std::string &msg)
{
	_players->DeleteAllItems();
	_bots->DeleteAllItems();
	_btnOK->SetEnabled(false);
	_buf->WriteLine(0, msg);
}

void WaitingForPlayersDlg::OnMessage(const std::string &msg)
{
	_buf->WriteLine(0, msg);
}

void WaitingForPlayersDlg::OnPlayerReady(unsigned short id, bool ready)
{
	assert(g_level);
	int count = g_level->GetList(LIST_players).size();
	assert(_players->GetItemCount() <= count); // count includes bots

	for( int index = 0; index < count; ++index )
	{
		GC_Player *player = (GC_Player *) _players->GetItemData(index);
		assert(player);
		assert(!player->IsKilled());
		assert(0 != player->GetNetworkID());

		if( player->GetNetworkID() == id )
		{
			_players->SetItemText(index, 3, ready ? g_lang->net_chatroom_player_ready->Get() : "");
			return;
		}
	}
	assert(false);
}

void WaitingForPlayersDlg::OnPlayersUpdate()
{
	size_t count = g_level->GetList(LIST_players).size();

	// TODO: implement via the ListDataSource interface
	_players->DeleteAllItems();
	_bots->DeleteAllItems();
	FOREACH(g_level->GetList(LIST_players), GC_Player, player)
	{
		if( GC_PlayerAI *ai = dynamic_cast<GC_PlayerAI *>(player) )
		{
			// nick & skin
			int index = _bots->AddItem(ai->GetNick());
			_bots->SetItemText(index, 1, ai->GetSkin());

			// team
			std::ostringstream tmp;
			tmp << g_lang->net_chatroom_team->Get() << ai->GetTeam();
			_bots->SetItemText(index, 2, tmp.str());

			// level
			assert(ai->GetLevel() <= AI_MAX_LEVEL);
			_bots->SetItemText(index, 3, g_lang.GetRoot()->GetStr(EditBotDlg::levels[ai->GetLevel()], NULL)->Get());
		}
		else
		{
			int index = _players->AddItem(player->GetNick(), (UINT_PTR) player);
			_players->SetItemText(index, 1, player->GetSkin());

			std::ostringstream tmp;
			tmp << g_lang->net_chatroom_team->Get() << player->GetTeam();
			_players->SetItemText(index, 2, tmp.str());
		}

//		_buf->printf(g_lang->net_chatroom_player_x_disconnected->Get().c_str(), player->GetNick().c_str());
//		_buf->printf(g_lang->net_chatroom_player_x_connected->Get().c_str(), player->GetNick().c_str());
//		_buf->printf("\n");
	}

	_btnOK->SetEnabled(_players->GetItemCount() > 0);
}

void WaitingForPlayersDlg::OnStartGame()
{
	Close(_resultOK);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
