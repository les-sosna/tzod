// gui_network.h

#pragma once

#include "Base.h"
#include "Dialog.h"

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
	Edit      *_svLatency;

public:
	CreateServerDlg(Window *parent);
	~CreateServerDlg();

protected:
	void OnOK();
	void OnCancel();
	void OnCloseChild(int result);
};

///////////////////////////////////////////////////////////////////////////////

class ConnectDlg : public Dialog
{
	Button *_btnOK;
	Edit   *_name;
	List   *_status;

	void Error(const char *msg);

public:
	ConnectDlg(Window *parent, const char *autoConnect);
	~ConnectDlg();

protected:
	void OnOK();
	void OnCancel();
	void OnTimeStep(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class WaitingForPlayersDlg : public Dialog
{
	List           *_players;
	Console        *_chat;
	ConsoleBuffer  *_buf;
	Button         *_btnOK;

public:
	WaitingForPlayersDlg(Window *parent);
	~WaitingForPlayersDlg();

protected:
	void OnOK();
	void OnCancel();
	void OnTimeStep(float dt);
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
