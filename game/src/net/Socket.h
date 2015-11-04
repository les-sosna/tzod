// Socket.h

#pragma once


class Socket
{
public:
	explicit Socket(SOCKET s = INVALID_SOCKET);
	~Socket();

	enum { OK, Error, Aborted };

	int EnumNetworkEvents(LPWSANETWORKEVENTS lpNetworkEvents);

	int SetEvents(long lNetworkEvents);  // return zero if the operation was successful
	int Close();

	void SetCallback(Delegate<void()> callback);

	operator SOCKET () const { return _socket; }
	void Attach(SOCKET s);

private:
	SOCKET   _socket;
	WSAEVENT _event;
	bool     _hasCallback;
};

///////////////////////////////////////////////////////////////////////////////
// end of file
