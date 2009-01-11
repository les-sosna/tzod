// gui_network.h

#pragma once

#include "Base.h"
#include "Dialog.h"

// forward declarations
class DataBlock;


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

// forward declarations
class MapList;

class CreateServerDlg : public Dialog
{
	MapList   *_maps;
	CheckBox  *_nightMode;
	Edit      *_gameSpeed;
	Edit      *_fragLimit;
	Edit      *_timeLimit;
	Edit      *_svFps;
//	Edit      *_svLatency;

public:
	CreateServerDlg(Window *parent);
	virtual ~CreateServerDlg();

protected:
	void OnOK();
	void OnCancel();
	virtual void OnCloseChild(int result);
};

///////////////////////////////////////////////////////////////////////////////

class ConnectDlg : public Dialog
{
	Button *_btnOK;
	Edit   *_name;
	List   *_status;
	bool    _auto;

	void Error(const char *msg);

public:
	ConnectDlg(Window *parent, const char *autoConnect);
	virtual ~ConnectDlg();

protected:
	void OnOK();
	void OnCancel();

	void OnNewData(const DataBlock &db);
};

///////////////////////////////////////////////////////////////////////////////

class WaitingForPlayersDlg : public Dialog
{
	List           *_players;
	List           *_bots;
	Console        *_chat;
	Button         *_btnOK;
	SafePtr<ConsoleBuffer>  _buf;

	static const size_t _maxPings = 5;
	std::vector<DWORD> _pings;

public:
	WaitingForPlayersDlg(Window *parent);
	virtual ~WaitingForPlayersDlg();

protected:
	void OnAddBot();
	void OnAddBotClose(int result);
	void OnOK();
	void OnCancel();
	void OnSendMessage(const char *msg);
	
	void OnNewData(const DataBlock &db);
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
