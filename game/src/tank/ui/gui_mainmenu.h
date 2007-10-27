// gui_mainmenu.h

#pragma once

#include "Base.h"
#include "Dialog.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class MainMenuDlg : public Dialog
{
	void OnSinglePlayer();
	void OnNewGame();
	void OnCampaign(string_t name);

	void OnMultiPlayer();
	void OnHost();
	void OnJoin();

	void OnEditor();
	void OnNewMap();

	void OnSettings();
	void OnExit();


	enum PanelType
	{
		PT_NONE,
		PT_SINGLEPLAYER,
		PT_MULTIPLAYER,
		PT_EDITOR,
	};

	enum PanelState
	{
		PS_NONE,
		PS_APPEARING,
		PS_DISAPPEARING,
	};

	Window    *_panel;
	PanelType  _ptype;
	PanelState _pstate;

	std::list<DelegateAdapter1<string_t> >  _campaigns;

public:
	MainMenuDlg(Window *parent);
	virtual void OnParentSize(float width, float height);
	virtual void OnRawChar(int c);

protected:
	void OnCloseChild(int result);
	bool ClearPanel(PanelType newtype); // return true if newtype != oldtype
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
