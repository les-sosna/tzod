// TankServer.h

#include "stdafx.h"

#include "core/debug.h"
#include "core/Application.h"

#include "config/Config.h"
#include "config/Language.h"

#include "TankServer.h"
#include "ServerFunctions.h"
#include "ClientFunctions.h"
#include "Level.h"

#include "LobbyClient.h"

#include "functions.h"

#include "gc/Player.h"

///////////////////////////////////////////////////////////////////////////////

PeerServer::PeerServer(SOCKET s_)
  : Peer(s_)
  , ctrlValid(false)
  , descValid(false)
  , svlatency(0)
  , clboost(1)
{
}

///////////////////////////////////////////////////////////////////////////////

TankServer::TankServer(const GameInfo &info, const SafePtr<LobbyClient> &announcer)
  : _connectedCount(0)
  , _frameReadyCount(0)
  , _gameInfo(info)
  , _announcer(announcer)
{
	g_app->InitNetwork();

	_socketListen.Attach(socket(PF_INET, SOCK_STREAM, 0));
	if( INVALID_SOCKET == _socketListen )
	{
		throw std::runtime_error("sv: Unable to create socket");
	}

	sockaddr_in addr = {0};
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons(g_conf.sv_port.GetInt());
	addr.sin_family      = AF_INET;

	if( bind(_socketListen, (sockaddr *) &addr, sizeof(sockaddr_in)) )
		throw std::runtime_error(std::string("[sv] Unable to bind socket - ") + StrFromErr(WSAGetLastError()));

	if( listen(_socketListen, SOMAXCONN) )
		throw std::runtime_error(std::string("[sv] Listen call failed - ") + StrFromErr(WSAGetLastError()));

	if( _socketListen.SetEvents(FD_ACCEPT) )
		throw std::runtime_error(std::string("[sv] Unable to select event - ") + StrFromErr(WSAGetLastError()));

	_socketListen.SetCallback(CreateDelegate(&TankServer::OnListenerEvent, this));

	if( _announcer )
		_announcer->AnnounceHost(g_conf.sv_port.GetInt());

	TRACE("Server is online!");
}

TankServer::~TankServer(void)
{
	TRACE("Server is shutting down");


	//
	// cancel lobby registration
	//

	if( _announcer )
		_announcer->Cancel();


	//
	// close listener
	//

	if( INVALID_SOCKET != _socketListen )
		_socketListen.Close();


	//
	// disconnect clients
	//

	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
		(*it)->Close();


	TRACE("sv: Server destroyed");
}

void TankServer::OnListenerEvent()
{
	WSANETWORKEVENTS ne = {0};
	if( _socketListen.EnumNetworkEvents(&ne) )
	{
		TRACE("sv: EnumNetworkEvents error 0x%08x", WSAGetLastError());
		return;
	}
	assert(ne.lNetworkEvents & FD_ACCEPT);

	if( 0 != ne.iErrorCode[FD_ACCEPT_BIT] )
	{
		TRACE("sv: accept error 0x%08x", ne.iErrorCode[FD_ACCEPT_BIT]);
		return;
	}


	SOCKET s = accept(_socketListen, NULL, NULL);
	if( INVALID_SOCKET == s )
	{
		TRACE("sv: accept call returned error 0x%08x", WSAGetLastError());
		return;
	}


	TRACE("sv: Client connected");


	//
	// Add new client
	//

	_clients.push_back(SafePtr<PeerServer>(new PeerServer(s)));
	PeerServer &cl = *_clients.back();

	cl.RegisterHandler<std::string>(SV_POST_TEXTMESSAGE, CreateDelegate(&TankServer::SvTextMessage, this));
	cl.RegisterHandler<ControlPacket>(SV_POST_CONTROL, CreateDelegate(&TankServer::SvControl, this));
	cl.RegisterHandler<bool>(SV_POST_PLAYERREADY, CreateDelegate(&TankServer::SvPlayerReady, this));
	cl.RegisterHandler<BotDesc>(SV_POST_ADDBOT, CreateDelegate(&TankServer::SvAddBot, this));
	cl.RegisterHandler<PlayerDesc>(SV_POST_PLAYERINFO, CreateDelegate(&TankServer::SvPlayerInfo, this));

	cl.descValid = false;
	cl.eventDisconnect.bind(&TankServer::OnDisconnect, this);

	// send server info
	cl.Post(CL_POST_GAMEINFO, Variant(_gameInfo));
}

void TankServer::BroadcastTextMessage(const std::string &msg)
{
	Variant broadcast(msg);
	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		(*it)->Post(CL_POST_TEXTMESSAGE, broadcast);
	}
}

void TankServer::OnDisconnect(Peer *who_, int err)
{
	TRACE("sv: client disconnected");

	assert(dynamic_cast<PeerServer*>(who_));
	PeerServer *who = static_cast<PeerServer*>(who_);

	if( who->descValid )
	{
		who->descValid = false;
		--_connectedCount;
		if( who->ctrlValid )
			--_frameReadyCount;

		Variant arg(g_level->GetList(LIST_players).IndexOf(&*who->player));
		for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
		{
			if( who->player != (*it)->player )
				(*it)->Post(CL_POST_PLAYERQUIT, arg);
		}

		if( _frameReadyCount == _connectedCount )
			SendFrame();
	}

	who->Close();
	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		if( (*it) == who )
		{
			_clients.erase(it);
			break;
		}
	}
}

void TankServer::SendFrame()
{
	assert(_frameReadyCount == _connectedCount);


	//
	// collect control packets from all connected clients
	//

	Variant arg;
	arg.ChangeType(VariantTypeId<ControlPacketVector>());
	ControlPacketVector &ctrl = arg.Value<ControlPacketVector>();
	ctrl.resize(_connectedCount);

	int i = 0;
	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		if( !(*it)->descValid ) continue;
		const ControlPacket &cp = (*it)->ctrl;
		++i;
		ctrl[_connectedCount - i] = cp;

#ifdef NETWORK_DEBUG
		FrameToCSMap::_Pairib ib = _frame2cs.insert(FrameToCSMap::value_type(cp.frame, cp.checksum));
		if( !ib.second && ib.first->second != cp.checksum )
		{
			TRACE("sv: sync error detected at frame %u!", cp.frame);
			char buf[256];
			sprintf(buf, "sync error at frame %u: 0x%x 0x%x", cp.frame, ib.first->second, cp.checksum);
			MessageBox(g_env.hMainWnd, buf, TXT_VERSION, MB_ICONERROR);
			ExitProcess(-1);
		}
#endif
	}


	//
	// broadcast control
	//

	for( PeerList::iterator it1 = _clients.begin(); it1 != _clients.end(); it1++ )
	{
		if( (*it1)->descValid )
		{
			(*it1)->Post(CL_POST_CONTROL, arg);
			(*it1)->Post(CL_POST_SETBOOST, Variant((*it1)->clboost));
		}
	}


	//
	// remove control packet from clients' queue
	//

	_frameReadyCount = 0;
	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		if( (*it)->descValid )
		{
			assert((*it)->ctrlValid);
			if( (*it)->svlatency < g_conf.sv_latency.GetInt() )
			{
				TRACE("sv: extra packet added");
				++(*it)->svlatency;
				++_frameReadyCount;
				if( _frameReadyCount == _connectedCount )
				{
					SendFrame();
				}
			}
			else
			{
				(*it)->ctrlValid = false;
				(*it)->Resume();
			}
		}
	}
}

std::string TankServer::GetStats() const
{
	std::stringstream s;
	for( PeerList::const_iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		if( (*it)->descValid )
		{
			s << (*it)->desc.nick << ":" << (*it)->leading.Count() << " (boost:" << (*it)->clboost << ") ";
		}
	}
	return s.str();
}

void TankServer::SvTextMessage(Peer *from, int task, const Variant &arg)
{
	PeerServer *who = static_cast<PeerServer *>(from);
	std::stringstream msg;
	msg << "<" << who->desc.nick << "> " << arg.Value<std::string>();
	BroadcastTextMessage(msg.str());
}

void TankServer::SvControl(Peer *from, int task, const Variant &arg)
{
	PeerServer *who = static_cast<PeerServer *>(from);
	assert(!who->ctrlValid);

	if( who->svlatency > g_conf.sv_latency.GetInt() )
	{
		TRACE("sv: extra packet skipped");
		--who->svlatency;
	}
	else
	{
		who->ctrlValid = true;
		who->ctrl = arg.Value<ControlPacket>();
		who->Pause();

		++_frameReadyCount;
		if( _frameReadyCount == _connectedCount )
		{
			SendFrame();
			assert(!_connectedCount || _frameReadyCount != _connectedCount);
			float sum = 0;
			for( PeerList::const_iterator it = _clients.begin(); it != _clients.end(); ++it )
			{
				if( (*it)->descValid )
				{
					(*it)->leading.Push((*it)->GetPending() > 0);
					(*it)->clboost -= g_conf.sv_sensitivity.GetFloat() * (float) (*it)->leading.Count() / (*it)->leading.GetCapacity();
					sum += (*it)->clboost;
				}
			}
			for( PeerList::const_iterator it = _clients.begin(); it != _clients.end(); ++it )
			{
				if( (*it)->descValid )
					(*it)->clboost /= sum / (float) _connectedCount;
			}
		}
	}
}

void TankServer::SvPlayerReady(Peer *from, int task, const Variant &arg)
{
	PeerServer *who = static_cast<PeerServer *>(from);

	PlayerReady reply = { g_level->GetList(LIST_players).IndexOf(who->player), arg.Value<bool>() };

	bool bAllPlayersReady = true;
	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		(*it)->Post(CL_POST_PLAYER_READY, Variant(reply));
		if( !(*it)->player->GetReady() )
			bAllPlayersReady = false;
	}

	if( bAllPlayersReady )
	{
		_socketListen.Close();
		if( _announcer )
			_announcer->Cancel();
		BroadcastTextMessage(g_lang.net_msg_starting_game.Get());
		for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
		{
			(*it)->Post(CL_POST_STARTGAME, Variant(true));
		}
	}
}

void TankServer::SvAddBot(Peer *from, int task, const Variant &arg)
{
	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		(*it)->Post(CL_POST_ADDBOT, arg);
	}
}

void TankServer::SvPlayerInfo(Peer *from, int task, const Variant &arg)
{
	PeerServer *who = static_cast<PeerServer *>(from);

	who->desc = arg.Value<PlayerDesc>();

	if( !who->descValid )
	{
		// TODO: tell newly connected player about other players
		assert(false);
//		for( size_t i = 0; i < _players.size(); ++i )
//		{
//			who->Post(_players[i].first, _players[i].second);
//		}
		who->descValid = true;
		++_connectedCount;
	}

	// broadcast to other players
	PlayerDescEx pde;
	pde.pd = who->desc;
	pde.idx = g_level->GetList(LIST_players).IndexOf(who->player);
	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		(*it)->Post(CL_POST_PLAYERINFO, Variant(pde));
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file
