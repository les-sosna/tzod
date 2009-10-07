// LobbyClient.cpp

#include "stdafx.h"
#include "LobbyClient.h"
#include "HttpClient.h"

#include "core/Application.h"
#include "core/debug.h"

#define LOBBY_KEY_LENGTH   10
#define LOBBY_VERSION      "149b"
#define LOBBY_MAX_REDIRECT 5

static std::string GenerateKey(unsigned int length)
{
	static const char dictionary[] = 
		"abcdefghigklmnopqrstuvwxwz"
		"ABCDEFGHIGKLMNOPQRSTUVWXWZ"
		"0123456789";

	assert(length > 0);

	std::string result;
	result.resize(length);

	for( unsigned int i = 0; i < length; ++i )
	{
		unsigned int u = 0;
		rand_s(&u);
		result[i] = dictionary[u % (sizeof(dictionary) - 1)];
	}

	return result;
}

static bool ParseServerList(std::vector<std::string> &result, const std::string &data)
{
	std::istringstream in(data);

	std::string s;
	while( std::getline(in, s) )
	{
		if( s == "end" )
		{
			return true;
		}
		result.push_back(s);
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////

LobbyClient::LobbyClient()
  : _sessionKey(GenerateKey(LOBBY_KEY_LENGTH))
  , _redirectCount(0)
  , _timer(CreateWaitableTimer(NULL, FALSE, NULL))
  , _state(STATE_IDLE)
{
	// setup timer
	if( !_timer || INVALID_HANDLE_VALUE == _timer )
	{
		throw std::runtime_error("lobby: failed to create waitable timer");
	}
	g_app->RegisterHandle(_timer, CreateDelegate(&LobbyClient::OnTimer, this));
}

LobbyClient::~LobbyClient()
{
	g_app->UnregisterHandle(_timer);
	CloseHandle(_timer);
}

void LobbyClient::ResetHttp()
{
	// recreate http client to cancel any current requests
	_http = WrapRawPtr(new HttpClient());
	_http->eventResult.bind(&LobbyClient::OnHttpResult, this);
}

void LobbyClient::SetLobbyUrl(const std::string &lobbyUrl)
{
	assert(STATE_IDLE == _state);
	_lobbyUrl = lobbyUrl;
}

void LobbyClient::RequestServerList()
{
	assert(STATE_IDLE == _state);
	_state = STATE_LIST;
	_redirectCount = 0;
	_param.clear();
	_param["ver"] = LOBBY_VERSION;

	ResetHttp();
	_http->Get(_lobbyUrl, _param);
}

void LobbyClient::AnnounceHost(int port)
{
	assert(STATE_IDLE == _state);
	_state = STATE_ANNOUNCE;

	std::stringstream s;
	s << port; // convert integer to string

	_redirectCount = 0;
	_param.clear();
	_param["ver"] = LOBBY_VERSION;
	_param["reg"] = s.str();
	_param["key"] = _sessionKey;

	ResetHttp();
	_http->Get(_lobbyUrl, _param);
}

void LobbyClient::Cancel()
{
	if( STATE_ANNOUNCE == _state )
	{
		_state = STATE_CANCEL;
		_redirectCount = 0;
		// use params from AnnounceHost call 
		_param["unreg"] = _param["reg"];
		_param.erase("reg");

		ResetHttp();
		_http->Get(_lobbyUrl, _param);

		// prevent client deletion until command completes
		AddRef();

		// set up the timer to force deletion if command hangs
		LARGE_INTEGER dueTime;
		dueTime.QuadPart = -100000000; // 10 seconds
		SetWaitableTimer(_timer, &dueTime, 0, NULL, NULL, FALSE);
	}
}

void LobbyClient::OnHttpResult(int err, const std::string &result, const HttpParam *headers)
{
	HttpClient::Param::const_iterator it;

	switch( err )
	{
		case 301: // moved permanently
			assert(STATE_IDLE != _state);
			++_redirectCount;
			it = headers->find("Location"); // TODO: case insensitive search
			if( headers->end() == it || _redirectCount > LOBBY_MAX_REDIRECT )
			{
				_state = STATE_IDLE;
				if( eventError )
				{
					INVOKE(eventError)("lobby: redirect failed");
				}
				return;
			}
			_lobbyUrl = it->second;
			_http->Get(_lobbyUrl, _param);
			break;

		case 200: // ok
			assert(STATE_IDLE != _state);
			switch( _state )
			{
				case STATE_ANNOUNCE:
				{
					if( result.substr(0, 2) == "ok" )
					{
						LARGE_INTEGER dueTime;
						dueTime.QuadPart = -300000000; // 30 seconds
						SetWaitableTimer(_timer, &dueTime, 0, NULL, NULL, FALSE);
					}
					else
					{
						_state = STATE_IDLE;
						TRACE("lobby: invalid server reply - %s", result.c_str());
						if( eventError )
						{
							INVOKE(eventError) ("invalid lobby reply");
						}
					}
					break;
				}

				case STATE_LIST:
				{
					_state = STATE_IDLE;
					std::vector<std::string> svlist;
					if( ParseServerList(svlist, result) )
					{
						if( eventServerListReply )
						{
							INVOKE(eventServerListReply) (svlist);
						}
					}
					else
					{
						TRACE("lobby: invalid server reply - %s", result.c_str());
						if( eventError )
						{
							INVOKE(eventError) ("invalid lobby reply");
						}
					}
					break;
				}

				case STATE_CANCEL:
				{
					CancelWaitableTimer(_timer);
					_state = STATE_IDLE;
					if( result.substr(0, 2) != "ok" )
					{
						TRACE("lobby: invalid server reply - %s", result.c_str());
						if( eventError )
						{
							INVOKE(eventError) ("invalid lobby reply");
						}
					}
					Release();
					break;
				}
			} // end of switch( _state )
			break;

		default:  // unknown error
			_state = STATE_IDLE;
			std::stringstream s;
			s << "lobby: error " << err;
			if( eventError )
			{
				INVOKE(eventError)(s.str());
			}
	}
}

void LobbyClient::OnTimer()
{
	if( STATE_ANNOUNCE == _state )
	{
		// refresh
		_redirectCount = 0;
		_http->Get(_lobbyUrl, _param);
	}
	else if( STATE_CANCEL == _state )
	{
		_state = STATE_IDLE;
		Release();
	}
}



// end of file
