// Socket.h

#pragma once


class Socket
{
	SOCKET   _socket;
	WSAEVENT _event;
	WSANETWORKEVENTS _ne;
	std::vector<char> _accum;

public:

	enum { OK, Error, Aborted };

	int Send(HANDLE hAbortEvent, const void *buf, int len);
	int Send_accum(HANDLE hAbortEvent);
	void Accumulate(const void *buf, int len);

	int Recv(HANDLE hAbortEvent, void *buf, int len);
	int Recv(const HANDLE *lphAbortEvents, size_t count, void *buf, int buflen);

	int Wait();
	int Wait(HANDLE hAbortEvent);
	int Wait(const HANDLE *lphAbortEvents, size_t count);

	int SetEvents(long lNetworkEvents);  // return zero if the operation was successful
	int Close();
	bool CheckEvent(int bit);

	operator SOCKET () const { return _socket; }
	SOCKET operator = (SOCKET s);

	Socket(void);
	~Socket(void);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
