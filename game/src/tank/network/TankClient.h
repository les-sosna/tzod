// TankClient.h

#pragma once


#include "Socket.h"
#include "datablock.h"
#include "Peer.h"

/////////////////////////////////////////////////////////

struct NetworkStats
{
	size_t  bytesSent;
	size_t  bytesRecv;
};

class TankClient
{
	SafePtr<Peer> _peer;

	int _frame;

	void Message(const string_t &msg, bool err = false);

	void OnDisconnect(Peer *, int err);
	void OnRecv(Peer *who, const DataBlock &db);

public:
	int _latency;

private:
	DWORD _clientId;
	NetworkStats _stats;

public:
	TankClient(void);
	~TankClient(void);

	DWORD GetId() const { return _clientId; }

	void Connect(const string_t &hostaddr);
	void ShutDown();

	void SendDataToServer(const DataBlock &data);

	void SendControl(const ControlPacket &cp); // вызов функции завершает кадр
	void GetStatistics(NetworkStats *pStats);

	Delegate<void(const DataBlock&)> eventNewData;
};

///////////////////////////////////////////////////////////////////////////////
// end of file
