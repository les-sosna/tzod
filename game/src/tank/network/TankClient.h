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

#define STATE_MOVEFORVARD	0x0001
#define STATE_MOVEBACK		0x0002
#define STATE_ROTATELEFT	0x0004
#define STATE_ROTATERIGHT	0x0008
#define STATE_FIRE			0x0010
#define STATE_ALLOWDROP		0x0020
#define STATE_TOWERLEFT		0x0040
#define STATE_TOWERRIGHT	0x0080
#define STATE_TOWERCENTER	0x0100
#define STATE_ENABLELIGHT	0x0200


#define WM_NEWDATA			(WM_USER + 1002)
#define WM_CUSTOMCLIENTMSG	(WM_USER + 1003)	// this message must be mirrored back


typedef struct NETWORKSTATS
{
	int    nFramesInBuffer;
	DWORD  dwBytesSent;
	DWORD  dwBytesRecv;
} NETWORKSTATS, *LPNETWORKSTATS;

class TankClient
{
	static const size_t BUFFER_SIZE = 4096;


	std::queue<DataBlock> _incoming;
	char _buf_incoming[BUFFER_SIZE];
	size_t _buf_incoming_size;

	std::queue<DataBlock> _outgoing;
	char _buf_outgoing[BUFFER_SIZE];
	size_t _buf_outgoing_size;

	HWND _hMainWnd;

	int _frame;


	bool _bReadyToSend;
	bool _bInit;

	bool recv_all();	// return false if an error accrues
	bool send_all();	// return false if an error accrues

	Socket   _socket;
	HWND   _hWnd;	// этому окну приходят сообщения о новых данных
	void NewData(const DataBlock &data);
	void Message(const char *msg, bool err = false);

public:
	DWORD _dwLatency;

private:


	DWORD _dwClientId;
	NETWORKSTATS _stats;

	HANDLE _evCheckPoint;

public:
	TankClient(void);
	~TankClient(void);

	DWORD GetId() const { return _dwClientId; }

	bool Connect(const char* hostaddr, HWND hMainWnd);
	void ShutDown();

	void SetWindow(HWND hWnd);
	LRESULT Mirror(WPARAM wParam, LPARAM lParam);
	bool GetData(DataBlock &data);

	void SendDataToServer(const DataBlock &data);

	//----------------
	void SendControl(const ControlPacket &cp); // вызов функции завершает кадр
	void GetStatistics(LPNETWORKSTATS lpStats);


	std::queue<ControlPacket> _ctrlBuf;
	bool RecvControl(ControlPacket &cp);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
