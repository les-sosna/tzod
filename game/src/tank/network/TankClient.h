// TankClient.h

#pragma once


#include "Socket.h"
#include "datablock.h"

/////////////////////////////////////////////////////////


struct VehicleState;

class ControlPacket
{
public:
	WORD  wControlState;
	float fTowerAngle;
	float fBodyAngle;
#ifdef NETWORK_DEBUG
	DWORD checksum;
#endif
	//--------------------------
	ControlPacket();
	//--------------------------
	void fromvs(const VehicleState &vs);
	void tovs(VehicleState &vs) const;
};

#define STATE_MOVEFORWARD   0x0001
#define STATE_MOVEBACK      0x0002
#define STATE_ROTATELEFT    0x0004
#define STATE_ROTATERIGHT   0x0008
#define STATE_FIRE          0x0010
#define STATE_ALLOWDROP     0x0020
#define STATE_TOWERLEFT     0x0040
#define STATE_TOWERRIGHT    0x0080
#define STATE_TOWERCENTER   0x0100
#define STATE_ENABLELIGHT   0x0200

#define WM_CUSTOMCLIENTMSG  (WM_USER + 1003) // this message must be mirrored back


struct NetworkStats
{
	int    nFramesInBuffer;
	size_t  bytesSent;
	size_t  bytesRecv;
};

class TankClient
{
	static const size_t MAX_BUFFER_SIZE = 4096;

	std::queue<DataBlock> _incoming;
	char _buf_incoming[MAX_BUFFER_SIZE];
	size_t _buf_incoming_size;

	std::queue<DataBlock> _outgoing;
	char _buf_outgoing[MAX_BUFFER_SIZE];
	size_t _buf_outgoing_size;

	HWND _hwnd;

	int _frame;

	bool _readyToSend;
	bool _init;

	ControlPacket _lastPacket;

	bool recv_all(); // return false if an error occurs
	bool send_all(); // return false if an error occurs

	Socket   _socket;
	void NewData(const DataBlock &data);
	void Message(const char *msg, bool err = false);

public:
	DWORD _latency;

private:
	DWORD _clientId;
	NetworkStats _stats;
	bool  _gameStarted;

public:
	TankClient(void);
	~TankClient(void);

	DWORD GetId() const { return _clientId; }

	bool Connect(const char* hostaddr, HWND hMainWnd);
	void ShutDown();

	LRESULT Mirror(WPARAM wParam, LPARAM lParam);
	bool GetData(DataBlock &data);

	void SendDataToServer(const DataBlock &data);

	void SendControl(const ControlPacket &cp); // вызов функции завершает кадр
	void GetStatistics(NetworkStats *pStats);

	std::queue<ControlPacket> _ctrlBuf; // FIXME: move to Level class
	bool RecvControl(ControlPacket &cp);

	bool IsGameStarted() const { return _gameStarted; }
};

///////////////////////////////////////////////////////////////////////////////
// end of file
