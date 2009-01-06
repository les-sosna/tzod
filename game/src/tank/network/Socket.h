// Socket.h

#pragma once


class Socket
{
public:
	Socket(void);
	~Socket(void);

	enum { OK, Error, Aborted };

	int EnumNetworkEvents(LPWSANETWORKEVENTS lpNetworkEvents);

	int SetEvents(long lNetworkEvents);  // return zero if the operation was successful
	int Close();

	HANDLE GetEvent() const { return _event; };

	operator SOCKET () const { return _socket; }
	SOCKET operator = (SOCKET s);

private:
	SOCKET   _socket;
	WSAEVENT _event;
};

///////////////////////////////////////////////////////////////////////////////
// end of file
