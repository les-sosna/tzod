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

public:
	CreateServerDlg(Window *parent);
	~CreateServerDlg();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
