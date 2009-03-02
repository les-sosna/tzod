// LobbyClient.cpp

#include "stdafx.h"
#include "LobbyClient.h"
#include "HttpClient.h"

#include "core/Application.h"
#include "core/debug.h"
#include "core/Console.h"

#define LOBBY_KEY_LENGTH   10
#define LOBBY_VERSION      "149b"
#define LOBBY_MAX_REDIRECT 5

static std::string GenerateKey(unsigned int length)
{
	static const char dictionary[] = 
		"abcdefghigklmnopqrstuvwxwz"
		"ABCDEFGHIGKLMNOPQRSTUVWXWZ"
		"0123456789-_!*'()";

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
  : _http(new HttpClient())
  , _sessionKey(GenerateKey(LOBBY_KEY_LENGTH))
  , _redirectCount(0)
  , _timer(CreateWaitableTimer(NULL, FALSE, NULL))
{
	// setup timer
	if( !_timer || INVALID_HANDLE_VALUE == _timer )
	{
		throw std::runtime_error("failed to create waitable timer");
	}
	Delegate<void()> d;
	d.bind(&LobbyClient::OnTimer, this);
	g_app->RegisterHandle(_timer, d);

	// setup http client
	_http->eventResult.bind(&LobbyClient::OnHttpResult, this);
}

LobbyClient::~LobbyClient()
{
	g_app->UnregisterHandle(_timer);
	CloseHandle(_timer);
}

void LobbyClient::RequestServerList(const std::string &lobbyAddress)
{
	_redirectCount = 0;
	_param.clear();
	_param["ver"] = LOBBY_VERSION;
	_http->Get(lobbyAddress, _param);
}

void LobbyClient::AnnounceHost(const std::string &lobbyAddress, int port)
{
	std::stringstream s;
	s << port; // convert integer to string

	_redirectCount = 0;
	_param.clear();
	_param["ver"] = LOBBY_VERSION;
	_param["reg"] = s.str();
	_param["key"] = _sessionKey;
	_http->Get(lobbyAddress, _param);
}

void LobbyClient::OnHttpResult(int err, const std::string &result, const HttpParam *headers)
{
	HttpClient::Param::const_iterator it;

	switch( err )
	{
		case 301: // moved permanently
		{
			++_redirectCount;
			it = headers->find("Location"); // TODO: case insensitive search
			if( headers->end() == it || _redirectCount > LOBBY_MAX_REDIRECT )
			{
				INVOKE(eventError)("LobbyClient: redirect failed");
			}
			_http->Get(it->second, _param);
			break;
		}

		case 200: // ok
		{
			std::vector<std::string> svlist;
			if( ParseServerList(svlist, result) )
			{
				INVOKE(eventServerListReply) (svlist);
			}
			else
			{
				TRACE("lobby: invalid server reply - %s\n", result.c_str());
				INVOKE(eventError) ("invalid lobby reply");
			}
			break;
		}

		default:  // unknown error
		{
			std::stringstream s;
			s << "LobbyClient: error " << err;
			INVOKE(eventError)(s.str());
		}
	}

	TRACE("http responce: %s\n", result.c_str());
}

void LobbyClient::OnTimer()
{
	TRACE("lobby timer\n");
}



// end of file
