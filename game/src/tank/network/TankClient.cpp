// TankClient.cpp
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TankClient.h"
#include "ClientFunctions.h"
#include "ServerFunctions.h"
#include "CommonTypes.h"
#include "ControlPacket.h"
#include "Peer.h"

#include "core/debug.h"
#include "core/Application.h"

#include "ui/gui_desktop.h"
#include "ui/GuiManager.h"

#include "gc/ai.h"
#include "gc/Player.h"

#include "config/Config.h"
#include "config/Language.h"

#include "fs/FileSystem.h"

#include "Macros.h"
#include "functions.h"
#include "LevelInterfaces.h"

///////////////////////////////////////////////////////////////////////////////

TankClient::TankClient(bool isLocal, ILevelController *levelController)
  : _frame(0)
  , _boost(1)
  , _gameStarted(false)
  , _latency(1)
  , _hasCtrl(false)
  , _levelController(levelController)
  , _isLocal(isLocal)
{
	ZeroMemory(&_stats, sizeof(NetworkStats));
}

TankClient::~TankClient(void)
{
	if( _peer )
	{
		_peer->Close();
		_peer = NULL;
	}
}

void TankClient::Connect(const string_t &hostaddr)
{
	g_app->InitNetwork();


	//
	// get ip address
	//

	std::istringstream buf(hostaddr);
	string_t sv;
	std::getline(buf, sv, ':');

	unsigned short port = g_conf.sv_port.GetInt();
	buf >> port;


	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);

	// try to convert string to ip address
	addr.sin_addr.s_addr = inet_addr(sv.c_str());

	if( addr.sin_addr.s_addr == INADDR_NONE )
	{
		// try to resolve string as host name
		hostent* he = gethostbyname(sv.c_str());
		if( NULL == he )
		{
			int err = WSAGetLastError();
			TRACE("client: ERROR - Unable to resolve IP-address (%u)", err);
			OnDisconnect(NULL, err ? err : -1);
			return;
		}
		addr.sin_addr.s_addr = *((u_long*)he->h_addr_list[0]);
	}


	//
	// connect
	//

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if( INVALID_SOCKET == s )
	{
		int err = WSAGetLastError();
		TRACE("cl: ERROR - Unable to create socket (%u)", WSAGetLastError());
		OnDisconnect(NULL, err ? err : -1);
		return;
	}

	TRACE("cl: connecting to %s", inet_ntoa(addr.sin_addr));
	ClTextMessage(NULL, -1, Variant(g_lang.net_msg_connecting.Get()));

	_peer = new Peer(s);
	_peer->eventDisconnect.bind(&TankClient::OnDisconnect, this);

	_peer->RegisterHandler<std::string>(CL_POST_TEXTMESSAGE, CreateDelegate(&TankClient::ClTextMessage, this));
	_peer->RegisterHandler<std::string>(CL_POST_ERRORMESSAGE, CreateDelegate(&TankClient::ClErrorMessage, this));
	_peer->RegisterHandler<GameInfo>(CL_POST_GAMEINFO, CreateDelegate(&TankClient::ClGameInfo, this));
	_peer->RegisterHandler<unsigned short>(CL_POST_PLAYERQUIT, CreateDelegate(&TankClient::ClPlayerQuit, this));
	_peer->RegisterHandler<ControlPacketVector>(CL_POST_CONTROL, CreateDelegate(&TankClient::ClControl, this));
	_peer->RegisterHandler<PlayerReady>(CL_POST_PLAYER_READY, CreateDelegate(&TankClient::ClPlayerReady, this));
	_peer->RegisterHandler<bool>(CL_POST_STARTGAME, CreateDelegate(&TankClient::ClStartGame, this));
	_peer->RegisterHandler<PlayerDesc>(CL_POST_ADDHUMAN, CreateDelegate(&TankClient::ClAddHuman, this));
	_peer->RegisterHandler<BotDesc>(CL_POST_ADDBOT, CreateDelegate(&TankClient::ClAddBot, this));
	_peer->RegisterHandler<PlayerDescEx>(CL_POST_PLAYERINFO, CreateDelegate(&TankClient::ClSetPlayerInfo, this));
	_peer->RegisterHandler<float>(CL_POST_SETBOOST, CreateDelegate(&TankClient::ClSetBoost, this));

	if( int err = _peer->Connect(&addr) )
	{
		OnDisconnect(NULL, err);
	}
}

void TankClient::OnDisconnect(Peer *peer, int err)
{
	// TODO: treat this as an error when disconnect is unexpected
	Variant arg(g_lang.net_msg_connection_failed.Get());
	if( err )
	{
		ClErrorMessage(peer, -1, arg);
	}
	else
	{
		ClTextMessage(peer, -1, arg);
	}
}

void TankClient::SendTextMessage(const std::string &msg)
{
	_peer->Post(SV_POST_TEXTMESSAGE, Variant(msg));
}

void TankClient::SendControl(const ControlPacket &cp)
{
	assert(_gameStarted);

	//if( g_conf.sv_latency.GetInt() < _latency && _latency > 1 )
	//{
	//	--_latency;
	//	TRACE("cl: packet skipped");
	//	return;     // skip packet
	//}

	_peer->Post(SV_POST_CONTROL, Variant(cp));
	_frame++;


	//if( g_conf.sv_latency.GetInt() > _latency )
	//{
	//	++_latency;
	//	SendControl(cp); // duplicate packet
	//	TRACE("cl: packet duplicated");
	//}
}

void TankClient::SendPlayerReady(bool ready)
{
	assert(!_gameStarted);
	_peer->Post(SV_POST_PLAYERREADY, Variant(ready));
}

void TankClient::SendAddBot(const BotDesc &bot)
{
	_peer->Post(SV_POST_ADDBOT, Variant(bot));
}

void TankClient::SendPlayerInfo(const PlayerDesc &pd)
{
	_peer->Post(SV_POST_PLAYERINFO, Variant(pd));
}

void TankClient::GetStatistics(NetworkStats *pStats)
{
	pStats->bytesSent = _peer->GetTrafficOut();
	pStats->bytesRecv = _peer->GetTrafficIn();
	pStats->bytesPending = _peer->GetPending();
	if( _hasCtrl )
	{
		pStats->bytesPending += 10;
	}

//	memcpy(pStats, &_stats, sizeof(NetworkStats));
}

void TankClient::ClGameInfo(Peer *from, int task, const Variant &arg)
{
	assert(!_gameStarted);

	const GameInfo &gi = arg.Value<GameInfo>();

	if( 0 != memcmp(gi.exeVer, g_md5.bytes, 16) )
	{
		ClErrorMessage(NULL, -1, Variant(g_lang.net_connect_error_server_version.Get()));
		return;
	}

	g_conf.sv_timelimit.SetInt(gi.timelimit);
	g_conf.sv_fraglimit.SetInt(gi.fraglimit);
	g_conf.sv_fps.SetInt(gi.server_fps);
	g_conf.sv_nightmode.Set(gi.nightmode);

	std::string path = DIR_MAPS;
	path += "\\";
	path += gi.cMapName;
	path += ".map";

	try
	{
		SafePtr<FS::File> f = g_fs->Open(path);

		MD5_CTX md5;

		{
			SafePtr<FS::MemMap> m = f->QueryMap();
			MD5Init(&md5);
			MD5Update(&md5, m->GetData(), m->GetSize());
			MD5Final(&md5);
		}

		if( 0 != memcmp(gi.mapVer, md5.digest, 16) )
		{
			ClErrorMessage(NULL, -1, Variant(g_lang.net_connect_error_map_version.Get()));
			return;
		}

		_levelController->Clear();
		_levelController->init_newdm(f->QueryStream(), gi.seed);
	}
	catch( const std::exception &e )
	{
		ClErrorMessage(NULL, -1, Variant(std::string(e.what())));
		return;
	}

	g_conf.cl_map.Set(gi.cMapName);
	g_conf.ui_showmsg.Set(true);

	if( eventConnected )
	{
		INVOKE(eventConnected) ();
	}
}

void TankClient::ClPlayerReady(Peer *from, int task, const Variant &arg)
{
	assert(!_gameStarted);
	if( eventPlayerReady )
	{
		INVOKE(eventPlayerReady) (arg.Value<PlayerReady>().playerIdx, arg.Value<PlayerReady>().ready);
	}
}

void TankClient::ClStartGame(Peer *from, int task, const Variant &arg)
{
	assert(!_gameStarted);
	_gameStarted = true;

	if( eventStartGame )
	{
		INVOKE(eventStartGame) ();
	}
}

void TankClient::ClSetPlayerInfo(Peer *from, int task, const Variant &arg)
{
	assert(!_gameStarted);

	const PlayerDescEx &pde = arg.Value<PlayerDescEx>();
	_levelController->SetPlayerInfo(pde.idx, pde.pd);

	if( eventPlayersUpdate )
	{
		INVOKE(eventPlayersUpdate) ();
	}
}

void TankClient::ClTextMessage(Peer *from, int task, const Variant &arg)
{
	if( g_gui )
	{
		static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->WriteLine(arg.Value<std::string>());
	}
	if( eventTextMessage )
	{
		INVOKE(eventTextMessage) (arg.Value<std::string>());
	}
}

void TankClient::ClErrorMessage(Peer *from, int task, const Variant &arg)
{
	if( g_gui )
	{
		static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->WriteLine(arg.Value<std::string>());
	}
	if( eventErrorMessage )
	{
		INVOKE(eventErrorMessage) (arg.Value<std::string>());
	}
}

void TankClient::ClPlayerQuit(Peer *from, int task, const Variant &arg)
{
	unsigned short idx = arg.Value<unsigned short>();
	_levelController->PlayerQuit(idx);

	if( eventPlayersUpdate )
	{
		INVOKE(eventPlayersUpdate) ();
	}
}

void TankClient::ClAddHuman(Peer *from, int task, const Variant &arg)
{
	_levelController->AddHuman(arg.Value<PlayerDesc>(), false);
	if( eventPlayersUpdate )
		INVOKE(eventPlayersUpdate) ();
}

void TankClient::ClAddBot(Peer *from, int task, const Variant &arg)
{
	_levelController->AddBot(arg.Value<BotDesc>());
	if( eventPlayersUpdate )
		INVOKE(eventPlayersUpdate) ();
}

void TankClient::ClControl(Peer *from, int task, const Variant &arg)
{
	assert(_gameStarted);
	assert(!_hasCtrl);

	_ctrl = arg.Value<ControlPacketVector>();
	_hasCtrl = true;
	_peer->Pause();
}

void TankClient::ClSetBoost(Peer *from, int task, const Variant &arg)
{
	_boost = arg.Value<float>();
}

bool TankClient::RecvControl(ControlPacketVector &result)
{
	if( !_hasCtrl )
	{
		_peer->Resume();
	}
	if( _hasCtrl )
	{
		result.swap(_ctrl);
		_hasCtrl = false;
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// end of file
