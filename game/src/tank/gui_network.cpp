// gui_network.cpp

#include "gui_network.h"

#include "gui.h"
#include "gui_maplist.h"

#include "md5.h"
#include "script.h"

#include "config/Config.h"
#include "config/Language.h"

#include "core/Debug.h"

//#include "network/TankServer.h"
//#include "network/TankClient.h"
//#include "network/LobbyClient.h"

#include "gc/Player.h"
#include "gc/World.h"
#include "gc/Macros.h"

#include <fs/FileSystem.h>
#include <ui/Button.h>
#include <ui/Edit.h>
#include <ui/Combo.h>
#include <ui/Console.h>
#include <ui/ConsoleBuffer.h>
#include <ui/DataSourceAdapters.h>
#include <ui/GuiManager.h>
#include <ui/Text.h>

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

CreateServerDlg::CreateServerDlg(Window *parent, World &world, FS::FileSystem &fs)
  : Dialog(parent, 770, 450)
  , _world(world)
  , _fs(fs)
{
	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang.net_server_title.Get(), alignTextCT);
	title->SetFont("font_default");

	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 20;
	float x4 = x3 + 20;

	//
	// map list
	//

	Text::Create(this, x1, 46, g_lang.choose_map.Get(), alignTextLT);

	_maps = MapListBox::Create(this, fs);
	_maps->Move(x1, 62);
	_maps->Resize(x2 - x1, 300);
	_maps->SetTabPos(0,   4); // name
	_maps->SetTabPos(1, 384); // size
	_maps->SetTabPos(2, 448); // theme
	_maps->SetCurSel(_maps->GetData()->FindItem(g_conf.cl_map.Get()), false);
	_maps->SetScrollPos(_maps->GetCurSel() - (_maps->GetNumLinesVisible() - 1) * 0.5f);
	GetManager().SetFocusWnd(_maps);


	//
	// settings
	//

	{
		float y =  56;

		_nightMode = CheckBox::Create(this, x3, y, g_lang.night_mode.Get());
		_nightMode->SetCheck( g_conf.cl_nightmode.Get() );


		Text::Create(this, x3, y+=30, g_lang.game_speed.Get(), alignTextLT);
		_gameSpeed = Edit::Create(this, x4, y+=15, 80);
		_gameSpeed->SetInt(g_conf.cl_speed.GetInt());

		Text::Create(this, x3, y+=30, g_lang.frag_limit.Get(), alignTextLT);
		_fragLimit = Edit::Create(this, x4, y+=15, 80);
		_fragLimit->SetInt(g_conf.cl_fraglimit.GetInt());

		Text::Create(this, x3, y+=30, g_lang.time_limit.Get(), alignTextLT);
		_timeLimit = Edit::Create(this, x4, y+=15, 80);
		_timeLimit->SetInt(g_conf.cl_timelimit.GetInt());

		Text::Create(this, x3+30, y+=30, g_lang.zero_no_limits.Get(), alignTextLT);

		Text::Create(this, x3, y+=40, g_lang.net_server_fps.Get(), alignTextLT);
		_svFps = Edit::Create(this, x4, y+=15, 100);
		_svFps->SetInt(g_conf.sv_fps.GetInt());
	}


	//
	// Lobby
	//
	{
		_lobbyEnable = CheckBox::Create(this, 32, 390, g_lang.net_server_use_lobby.Get());
		_lobbyList = DefaultComboBox::Create(this);
		_lobbyList->Move(32, 415);
		_lobbyList->Resize(200);
		_lobbyAdd = Button::Create(this, g_lang.net_server_add_lobby.Get(), 250, 410);

		_lobbyEnable->SetCheck(g_conf.sv_use_lobby.Get());
		_lobbyList->SetEnabled(_lobbyEnable->GetCheck());
		_lobbyAdd->SetEnabled(_lobbyEnable->GetCheck());
		for( size_t i = 0; i < g_conf.lobby_servers.GetSize(); ++i )
		{
			const std::string &lobbyAddr = g_conf.lobby_servers.GetStr(i, "")->Get();
			_lobbyList->GetData()->AddItem(lobbyAddr);
			if( !i || g_conf.sv_lobby.Get() == lobbyAddr )
			{
				_lobbyList->SetCurSel(i);
			}
		}
		_lobbyList->GetList()->AlignHeightToContent(128);

		_lobbyEnable->eventClick = std::bind(&CreateServerDlg::OnLobbyEnable, this);
	}

	Button *btn;
	btn = Button::Create(this, g_lang.net_server_ok.Get(), 544, 410);
	btn->eventClick = std::bind(&CreateServerDlg::OnOK, this);

	btn = Button::Create(this, g_lang.net_server_cancel.Get(), 656, 410);
	btn->eventClick = std::bind(&CreateServerDlg::OnCancel, this);
}

CreateServerDlg::~CreateServerDlg()
{
}

void CreateServerDlg::OnOK()
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

//	std::shared_ptr<LobbyClient> announcer;
	g_conf.sv_use_lobby.Set(_lobbyEnable->GetCheck());
	if( _lobbyEnable->GetCheck() )
	{
		if( -1 == _lobbyList->GetCurSel() )
		{
			return;
		}
		g_conf.sv_lobby.Set(_lobbyList->GetData()->GetItemText(_lobbyList->GetCurSel(), 0));
//		announcer = new LobbyClient();
//		announcer->SetLobbyUrl(g_conf.sv_lobby.Get());
	}

//	SAFE_DELETE(g_client);


	std::string path = DIR_MAPS;
	path += "/";
	path += fn + ".map";

//	GameInfo gi;
//    memset(&gi, 0, sizeof(gi));
//	gi.seed       = rand();
//	gi.fraglimit  = std::max(0, std::min(MAX_FRAGLIMIT, _fragLimit->GetInt()));
//	gi.timelimit  = std::max(0, std::min(MAX_TIMELIMIT, _timeLimit->GetInt()));
//	gi.server_fps = std::max(MIN_NETWORKSPEED, std::min(MAX_NETWORKSPEED, _svFps->GetInt()));
//	gi.nightmode  = _nightMode->GetCheck();
//	strcpy(gi.cMapName, fn.c_str());
//	strcpy(gi.cServerName, "ZOD Server");

	try
	{
		std::shared_ptr<FS::MemMap> m = _fs.Open(path)->QueryMap();
		MD5_CTX md5;
		MD5Init(&md5);
		MD5Update(&md5, m->GetData(), m->GetSize());
		MD5Final(&md5);
//		memcpy(gi.mapVer, md5.digest, 16);

		assert(false);
	//	g_server = new TankServer(gi, announcer); // integrate into client instead
	}
	catch( const std::exception &e )
	{
		TRACE("%s", e.what());
//		MessageBox(g_env.hMainWnd, g_lang.net_server_error.Get().c_str(), TXT_VERSION, MB_OK|MB_ICONERROR);
		return;
	}

//	g_conf.sv_latency.SetInt(1);

//	assert(false); // dont connect; integrate server into client instead
//	(new ConnectDlg(GetParent()))->eventClose = std::bind(&CreateServerDlg::OnCloseChild, this, _1);

//	PauseGame(true);


//	SAFE_DELETE(g_client);

	assert(_world.IsEmpty());
//	new TankClient(_world);

	(new WaitingForPlayersDlg(GetParent(), _world))->eventClose =
        std::bind(&CreateServerDlg::OnCloseChild, this, std::placeholders::_1);

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

ConnectDlg::ConnectDlg(Window *parent, const std::string &defaultName, World &world)
  : Dialog(parent, 512, 384)
  , _world(world)
{
	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang.net_connect_title.Get(), alignTextCT);
	title->SetFont("font_default");


	Text::Create(this, 20, 65, g_lang.net_connect_address.Get(), alignTextLT);
	_name = Edit::Create(this, 25, 80, 300);
	_name->SetText(defaultName);


	Text::Create(this, 20, 105, g_lang.net_connect_status.Get(), alignTextLT);
	_status = DefaultListBox::Create(this);
	_status->Move(25, 120);
	_status->Resize(400, 180);

	_btnOK = Button::Create(this, g_lang.net_connect_ok.Get(), 312, 350);
	_btnOK->eventClick = std::bind(&ConnectDlg::OnOK, this);

	Button::Create(this, g_lang.net_connect_cancel.Get(), 412, 350)->eventClick = std::bind(&ConnectDlg::OnCancel, this);

	GetManager().SetFocusWnd(_name);
}

ConnectDlg::~ConnectDlg()
{
}

void ConnectDlg::OnOK()
{
	_status->GetData()->DeleteAllItems();

	_btnOK->SetEnabled(false);
	_name->SetEnabled(false);

//	SAFE_DELETE(g_client);

	assert(_world.IsEmpty());
//	TankClient *cl = new TankClient(_world);
//	_clientSubscribtion = cl->AddListener(this);
//	cl->Connect(_name->GetText());
}

void ConnectDlg::OnCancel()
{
	Close(_resultCancel);
}

void ConnectDlg::OnConnected()
{
	g_conf.cl_server.Set(_name->GetText());
	(new WaitingForPlayersDlg(GetParent(), _world))->eventClose = eventClose;
	Close(-1); // close with any code except ok and cancel
}

void ConnectDlg::OnErrorMessage(const std::string &msg)
{
	_status->GetData()->AddItem(msg);
//	SAFE_DELETE(g_client);
	_btnOK->SetEnabled(true);
	_name->SetEnabled(true);
}

void ConnectDlg::OnTextMessage(const std::string &msg)
{
	_status->GetData()->AddItem(msg);
}

void ConnectDlg::OnClientDestroy()
{
	_clientSubscribtion.reset();
}

///////////////////////////////////////////////////////////////////////////////

InternetDlg::InternetDlg(Window *parent, World &world)
  : Dialog(parent, 450, 384)
  , _world(world)
//  , _client(new LobbyClient())
{
//	_client->eventError.bind(&InternetDlg::OnLobbyError, this);
//	_client->eventServerListReply.bind(&InternetDlg::OnLobbyList, this);

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang.net_internet_title.Get(), alignTextCT);
	title->SetFont("font_default");


	Text::Create(this, 20, 65, g_lang.net_internet_lobby_address.Get(), alignTextLT);
	_name = Edit::Create(this, 25, 80, 300);
	_name->SetText("tzod.fatal.ru/lobby/");


	Text::Create(this, 20, 105, g_lang.net_internet_server_list.Get(), alignTextLT);
	_servers = DefaultListBox::Create(this);
	_servers->Move(25, 120);
	_servers->Resize(400, 180);
	_servers->eventChangeCurSel = std::bind(&InternetDlg::OnSelectServer, this, std::placeholders::_1);
	_status = Text::Create(_servers, _servers->GetWidth() / 2, _servers->GetHeight() / 2, "", alignTextCC);
	_status->SetFontColor(0x7f7f7f7f);


	_btnRefresh = Button::Create(this, g_lang.net_internet_refresh.Get(), 25, 320);
	_btnRefresh->eventClick = std::bind(&InternetDlg::OnRefresh, this);

	_btnConnect = Button::Create(this, g_lang.net_internet_connect.Get(), 175, 320);
	_btnConnect->eventClick = std::bind(&InternetDlg::OnConnect, this);
	_btnConnect->SetEnabled(false);

	Button::Create(this, g_lang.net_internet_cancel.Get(), 325, 320)->eventClick = std::bind(&InternetDlg::OnCancel, this);

	GetManager().SetFocusWnd(_name);

	OnRefresh();
}

InternetDlg::~InternetDlg()
{
}

void InternetDlg::OnRefresh()
{
	_servers->GetData()->DeleteAllItems();
	_status->SetText(g_lang.net_internet_searching.Get());

	_btnRefresh->SetEnabled(false);
	_name->SetEnabled(false);

//	_client->SetLobbyUrl(_name->GetText());
//	_client->RequestServerList();
}

void InternetDlg::OnConnect()
{
	if( -1 != _servers->GetCurSel() )
	{
		const std::string &addr = _servers->GetData()->GetItemText(_servers->GetCurSel(), 0);
		ConnectDlg *dlg = new ConnectDlg(GetParent(), addr, _world);
		dlg->eventClose = std::bind(&InternetDlg::OnCloseChild, this, std::placeholders::_1);
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
	_status->SetText(result.empty() ? g_lang.net_internet_not_found.Get() : "");
	for( size_t i = 0; i < result.size(); ++i )
	{
		_servers->GetData()->AddItem(result[i]);
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

struct PlayerDesc
{
    std::string nick;
    std::string skin;
    std::string cls;
    unsigned int team;
};

static PlayerDesc GetPlayerDescFromConf(const ConfPlayerBase &p)
{
	PlayerDesc result;
	result.nick = p.nick.Get();
	result.cls = p.platform_class.Get();
	result.skin = p.skin.Get();
	result.team = p.team.GetInt();
	return result;
}

WaitingForPlayersDlg::WaitingForPlayersDlg(Window *parent, World &world)
  : Dialog(parent, 680, 512)
  , _players(NULL)
  , _bots(NULL)
  , _chat(NULL)
  , _btnOK(NULL)
  , _btnProfile(NULL)
  , _buf(new UI::ConsoleBuffer(80, 500))
  , _world(world)
//  , _clientSubscribtion(g_client->AddListener(this))
{
	//
	// create controls
	//

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang.net_chatroom_title.Get(), alignTextCT);
	title->SetFont("font_default");

	Text::Create(this, 20, 50, g_lang.net_chatroom_players.Get(), alignTextLT);
	_players = DefaultListBox::Create(this);
	_players->Move(20, 65);
	_players->Resize(512, 70);
	_players->SetTabPos(1, 200);
	_players->SetTabPos(2, 300);
	_players->SetTabPos(3, 400);

	_btnProfile = Button::Create(this, g_lang.net_chatroom_my_profile.Get(), 560, 65);
	_btnProfile->eventClick = std::bind(&WaitingForPlayersDlg::OnChangeProfileClick, this);

	Text::Create(this, 20, 150, g_lang.net_chatroom_bots.Get(), alignTextLT);
	_bots = DefaultListBox::Create(this);
	_bots->Move(20, 165);
	_bots->Resize(512, 100);
	_bots->SetTabPos(1, 200);
	_bots->SetTabPos(2, 300);
	_bots->SetTabPos(3, 400);

	Button::Create(this, g_lang.net_chatroom_bot_new.Get(), 560, 180)->eventClick = std::bind(&WaitingForPlayersDlg::OnAddBotClick, this);


	Text::Create(this, 20, 285, g_lang.net_chatroom_chat_window.Get(), alignTextLT);

	_chat = Console::Create(this, 20, 300, 512, 200, _buf.get());
	_chat->SetTexture("ui/list", false);
	_chat->SetEcho(false);
	_chat->eventOnSendCommand = std::bind(&WaitingForPlayersDlg::OnSendMessage, this, std::placeholders::_1);


	_btnOK = Button::Create(this, g_lang.net_chatroom_ready_button.Get(), 560, 450);
	_btnOK->eventClick = std::bind(&WaitingForPlayersDlg::OnOK, this);
	_btnOK->SetEnabled(false);

	Button::Create(this, g_lang.common_cancel.Get(), 560, 480)->eventClick = std::bind(&WaitingForPlayersDlg::OnCancel, this);


	//
	// send player info
	//

//	dynamic_cast<TankClient*>(g_client)->SendPlayerInfo(GetPlayerDescFromConf(g_conf.cl_playerinfo));

	// send ping request
//	DWORD t = timeGetTime();
//	g_client->SendDataToServer(DataWrap(t, DBTYPE_PING));
}

WaitingForPlayersDlg::~WaitingForPlayersDlg()
{
}

void WaitingForPlayersDlg::OnCloseProfileDlg(int result)
{
	_btnProfile->SetEnabled(true);
	_btnOK->SetEnabled(true);

	if( _resultOK == result )
	{
//		dynamic_cast<TankClient*>(g_client)->SendPlayerInfo(GetPlayerDescFromConf(g_conf.cl_playerinfo));
	}
}

void WaitingForPlayersDlg::OnChangeProfileClick()
{
	_btnProfile->SetEnabled(false);
	_btnOK->SetEnabled(false);
	EditPlayerDlg *dlg = new EditPlayerDlg(GetParent(), g_conf.cl_playerinfo->GetRoot());
	dlg->eventClose = std::bind(&WaitingForPlayersDlg::OnCloseProfileDlg, this, std::placeholders::_1);
}

void WaitingForPlayersDlg::OnAddBotClick()
{
	(new EditBotDlg(this, g_conf.ui_netbotinfo->GetRoot()))->eventClose = std::bind(&WaitingForPlayersDlg::OnAddBotClose, this, std::placeholders::_1);
}

void WaitingForPlayersDlg::OnAddBotClose(int result)
{
	if( _resultOK == result )
	{
//		BotDesc bd;
//		bd.pd = GetPlayerDescFromConf(g_conf.ui_netbotinfo);
//		bd.level = g_conf.ui_netbotinfo.level.GetInt();
//		dynamic_cast<TankClient*>(g_client)->SendAddBot(bd);
	}
}

void WaitingForPlayersDlg::OnOK()
{
//	dynamic_cast<TankClient*>(g_client)->SendPlayerReady(true);
}

void WaitingForPlayersDlg::OnCancel()
{
//	SAFE_DELETE(g_client);

	Close(_resultCancel);
}

void WaitingForPlayersDlg::OnSendMessage(const std::string &msg)
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
//			dynamic_cast<TankClient*>(g_client)->SendTextMessage(msg);
		}
	}
}

void WaitingForPlayersDlg::OnErrorMessage(const std::string &msg)
{
	_players->GetData()->DeleteAllItems();
	_bots->GetData()->DeleteAllItems();
	_btnOK->SetEnabled(false);
	_buf->WriteLine(0, msg);
}

void WaitingForPlayersDlg::OnTextMessage(const std::string &msg)
{
	_buf->WriteLine(0, msg);
}

void WaitingForPlayersDlg::OnPlayerReady(size_t idx, bool ready)
{
	int count = _world.GetList(LIST_players).size();
	assert(_players->GetData()->GetItemCount() <= count); // count includes bots

//	for( int index = 0; index < count; ++index )
//	{
//		GC_Player *player = (GC_Player *) _players->GetData()->GetItemData(index);
//		assert(player);
//
//		if( _world.GetList(LIST_players).IndexOf(player) == idx )
//		{
//			_players->GetData()->SetItemText(index, 3, ready ? g_lang.net_chatroom_player_ready.Get() : "");
//			return;
//		}
//	}
	assert(false);
}

void WaitingForPlayersDlg::OnPlayersUpdate()
{
	// TODO: implement via the ListDataSource interface
	_players->GetData()->DeleteAllItems();
	_bots->GetData()->DeleteAllItems();
	FOREACH(_world.GetList(LIST_players), GC_Player, player)
	{
		if( !player->GetIsHuman() )
		{
			// nick & skin
			int index = _bots->GetData()->AddItem(player->GetNick());
			_bots->GetData()->SetItemText(index, 1, player->GetSkin());

			// team
			std::ostringstream tmp;
			tmp << g_lang.net_chatroom_team.Get() << player->GetTeam();
			_bots->GetData()->SetItemText(index, 2, tmp.str());

			// level
		//	assert(player->GetLevel() <= AI_MAX_LEVEL);
		//	_bots->GetData()->SetItemText(index, 3, g_lang->GetRoot()->GetStr(EditBotDlg::levels[player->GetLevel()], "")->Get());
		}
		else
		{
			int index = _players->GetData()->AddItem(player->GetNick(), (size_t) player);
			_players->GetData()->SetItemText(index, 1, player->GetSkin());

			std::ostringstream tmp;
			tmp << g_lang.net_chatroom_team.Get() << player->GetTeam();
			_players->GetData()->SetItemText(index, 2, tmp.str());
		}

//		_buf->printf(g_lang.net_chatroom_player_x_disconnected.Get().c_str(), player->GetNick().c_str());
//		_buf->printf(g_lang.net_chatroom_player_x_connected.Get().c_str(), player->GetNick().c_str());
//		_buf->printf("\n");
	}

	_btnOK->SetEnabled(_players->GetData()->GetItemCount() > 0);
}

void WaitingForPlayersDlg::OnStartGame()
{
	Close(_resultOK);
}

void WaitingForPlayersDlg::OnClientDestroy()
{
	_clientSubscribtion.reset();
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
