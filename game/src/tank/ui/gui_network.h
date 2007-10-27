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
};

///////////////////////////////////////////////////////////////////////////////

class ConnectDlg : public Dialog
{
	Button *_btnOK;
	Edit   *_name;

public:
	ConnectDlg(Window *parent);
	~ConnectDlg();

protected:
	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
