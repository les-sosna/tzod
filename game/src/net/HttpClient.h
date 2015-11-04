// HttpClient.h

#pragma once

#include "Socket.h"

class HttpClient : public RefCounted
{
public:
	typedef std::map<std::string, std::string> Param;


	HttpClient();
	virtual ~HttpClient();

	void Get(const std::string &url, const Param &param);
	Delegate<void(int err, const std::string &result, const Param *headers)> eventResult;

private:
	void OnSocketEvent();
	void Send(const std::string &msg);

	Socket _socket;
	std::vector<char> _outgoing;
	std::string _incoming;

	std::string _host;
	std::string _query;

	bool _connected;
};

// end of file
