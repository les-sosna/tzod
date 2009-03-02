// LobbyClient.h

#pragma once

// forward declarations
class HttpClient;


class LobbyClient : public RefCounted
{
public:
	LobbyClient();
	virtual ~LobbyClient();

	void RequestServerList(const std::string &lobbyAddress);
	void AnnounceHost(const std::string &lobbyAddress, int port);

	Delegate<void(const std::vector<std::string>&)> eventServerListReply;
	Delegate<void(const std::string&)> eventError;

private:
	typedef std::map<std::string, std::string> HttpParam;
	HttpParam _param;
	SafePtr<HttpClient> _http;
	std::string _sessionKey;
	int _redirectCount;
	HANDLE _timer;

	void OnHttpResult(int err, const std::string &result, const HttpParam *headers);

	void OnTimer();
};


// end of file
