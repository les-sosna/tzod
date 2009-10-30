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
#include "Level.h"

///////////////////////////////////////////////////////////////////////////////

TankClient::TankClient(void)
  : _frame(0)
  , _clientId(0)
  , _boost(1)
  , _gameStarted(false)
  , _latency(1)
  , _hasCtrl(false)
{
	ZeroMemory(&_stats, sizeof(NetworkStats));
}

TankClient::~TankClient(void)
{
	ShutDown();
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

	_peer = WrapRawPtr(new Peer(s));
	_peer->eventDisconnect.bind(&TankClient::OnDisconnect, this);

	_peer->RegisterHandler(CL_POST_TEXTMESSAGE, VariantTypeId<std::string>(), CreateDelegate(&TankClient::ClTextMessage, this));
	_peer->RegisterHandler(CL_POST_ERRORMESSAGE, VariantTypeId<std::string>(), CreateDelegate(&TankClient::ClErrorMessage, this));
	_peer->RegisterHandler(CL_POST_GAMEINFO, VariantTypeId<GameInfo>(), CreateDelegate(&TankClient::ClGameInfo, this));
	_peer->RegisterHandler(CL_POST_SETID, VariantTypeId<unsigned short>(), CreateDelegate(&TankClient::ClSetId, this));
	_peer->RegisterHandler(CL_POST_PLAYERQUIT, VariantTypeId<unsigned short>(), CreateDelegate(&TankClient::ClPlayerQuit, this));
	_peer->RegisterHandler(CL_POST_CONTROL, VariantTypeId<ControlPacketVector>(), CreateDelegate(&TankClient::ClControl, this));
	_peer->RegisterHandler(CL_POST_PLAYER_READY, VariantTypeId<PlayerReady>(), CreateDelegate(&TankClient::ClPlayerReady, this));
	_peer->RegisterHandler(CL_POST_STARTGAME, VariantTypeId<bool>(), CreateDelegate(&TankClient::ClStartGame, this));
	_peer->RegisterHandler(CL_POST_ADDBOT, VariantTypeId<BotDesc>(), CreateDelegate(&TankClient::ClAddBot, this));
	_peer->RegisterHandler(CL_POST_PLAYERINFO, VariantTypeId<PlayerDescEx>(), CreateDelegate(&TankClient::ClSetPlayerInfo, this));
	_peer->RegisterHandler(CL_POST_SETBOOST, VariantTypeId<float>(), CreateDelegate(&TankClient::ClSetBoost, this));

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

void TankClient::ShutDown()
{
	if( _peer )
	{
		_peer->Close();
		_peer = NULL;
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

		g_level->Clear();
		g_level->init_newdm(f->QueryStream(), gi.seed);
	}
	catch( const std::exception &e )
	{
		ClErrorMessage(NULL, -1, Variant(std::string(e.what())));
		return;
	}

//	g_level->PauseLocal(true); // paused until game is started

	g_conf.cl_map.Set(gi.cMapName);
	g_conf.ui_showmsg.Set(true);

	if( eventConnected )
	{
		INVOKE(eventConnected) ();
	}
}

void TankClient::ClSetId(Peer *from, int task, const Variant &arg)
{
	assert(!_gameStarted);
	_clientId = arg.Value<unsigned short>();
}

void TankClient::ClPlayerReady(Peer *from, int task, const Variant &arg)
{
	assert(!_gameStarted);
	if( eventPlayerReady )
	{
		INVOKE(eventPlayerReady) (arg.Value<PlayerReady>().id, arg.Value<PlayerReady>().ready);
	}
}

void TankClient::ClStartGame(Peer *from, int task, const Variant &arg)
{
	assert(!_gameStarted);
	_gameStarted = true;


//	g_level->PauseLocal(false);

	if( eventStartGame )
	{
		INVOKE(eventStartGame) ();
	}
}

void TankClient::ClSetPlayerInfo(Peer *from, int task, const Variant &arg)
{
	assert(!_gameStarted);

	const PlayerDescEx &pde = arg.Value<PlayerDescEx>();

	GC_Player *player = NULL;


	//
	// find existing player or create new one
	//

	FOREACH(g_level->GetList(LIST_players), GC_Player, p)
	{
		if( p->GetNetworkID() == pde.id )
		{
			player = p;
			break;
		}
	}

	if( !player )
	{
		if( GetId() == pde.id )
		{
			player = new GC_PlayerLocal();
			const string_t &profile = g_conf.cl_playerinfo.profile.Get();
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
			player = new GC_PlayerRemote(pde.id);
		}
	}

	player->SetClass(pde.pd.cls);
	player->SetNick(pde.pd.nick);
	player->SetSkin(pde.pd.skin);
	player->SetTeam(pde.pd.team);
	player->UpdateSkin();

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
	unsigned short id = arg.Value<unsigned short>();

	ObjectList::iterator it = g_level->GetList(LIST_players).begin();
	while( it != g_level->GetList(LIST_players).end() )
	{
		if( GC_PlayerRemote *p = dynamic_cast<GC_PlayerRemote*>(*it) )
		{
			if( p->GetNetworkID() == id )
			{
				if( g_gui )
				{
					static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->WriteLine(g_lang.msg_player_quit.Get());
				}
				p->Kill();
				break;
			}
		}
		++it;
	}

	if( eventPlayersUpdate )
	{
		INVOKE(eventPlayersUpdate) ();
	}
}

void TankClient::ClAddBot(Peer *from, int task, const Variant &arg)
{
	const BotDesc &bd = arg.Value<BotDesc>();
	GC_PlayerAI *ai = new GC_PlayerAI();

	ai->SetClass(bd.pd.cls);
	ai->SetNick(bd.pd.nick);
	ai->SetSkin(bd.pd.skin);
	ai->SetTeam(bd.pd.team);
	ai->SetLevel(std::max(0U, std::min(AI_MAX_LEVEL, bd.level)));
	ai->UpdateSkin();

	if( eventPlayersUpdate )
	{
		INVOKE(eventPlayersUpdate) ();
	}
}

void TankClient::ClControl(Peer *from, int task, const Variant &arg)
{
	assert(_gameStarted);
	assert(g_level);
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
