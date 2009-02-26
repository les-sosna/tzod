// LobbyClient.h

#pragma once

// forward declarations
class HttpClient;


class LobbyClient : public RefCounted
{
public:
	LobbyClient();
	virtual ~LobbyClient();

private:
	SafePtr<HttpClient> _http;
	HANDLE _timer;

	typedef std::map<std::string, std::string> HttpParam;
	void OnResult(int err, const std::string &result, const HttpParam *headers);

	void OnTimer();
};


// end of file
