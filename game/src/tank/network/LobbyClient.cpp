// LobbyClient.cpp

#include "stdafx.h"
#include "LobbyClient.h"
#include "HttpClient.h"

#include "core/Application.h"
#include "core/debug.h"
#include "core/Console.h"


LobbyClient::LobbyClient()
  : _http(new HttpClient())
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
	_http->eventResult.bind(&LobbyClient::OnResult, this);

	HttpClient::Param param;
	param["ver"] = "149b";
//	param["reg"] = "1945";
//	param["key"] = "0123456789";
	_http->Get("tzod.fatal.ru/lobby/", param);
}

LobbyClient::~LobbyClient()
{
	CloseHandle(_timer);
}

void LobbyClient::OnResult(int err, const std::string &result, const HttpParam *headers)
{
	TRACE("http responce: %s\n", result.c_str());
}

void LobbyClient::OnTimer()
{
	TRACE("lobby timer\n");
}



// end of file
