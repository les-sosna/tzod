// LobbyClient.h

#pragma once

// forward declarations
class HttpClient;


class LobbyClient : public RefCounted
{
public:
	LobbyClient();
	virtual ~LobbyClient();

	void SetLobbyUrl(const std::string &lobbyUrl);

	void RequestServerList();
	void AnnounceHost(int port);

	Delegate<void()> eventAnnounceActive;
	Delegate<void(const std::vector<std::string>&)> eventServerListReply;
	Delegate<void(const std::string&)> eventError;

private:
	enum State
	{
		STATE_IDLE,
		STATE_LIST,
		STATE_ANNOUNCE
	};
	typedef std::map<std::string, std::string> HttpParam;
	HttpParam _param;
	SafePtr<HttpClient> _http;
	std::string _sessionKey;
	std::string _lobbyUrl;
	int _redirectCount;
	HANDLE _timer;
	State _state;

	void OnHttpResult(int err, const std::string &result, const HttpParam *headers);

	void OnTimer();
};


// end of file
