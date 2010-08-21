// gui_mainmenu.h

#pragma once

#include "Base.h"
#include "Dialog.h"
#include "gc/MessageBox.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

// forward declarations
class GetFileNameDlg;


class MainMenuDlg : public Dialog
{
	void OnSinglePlayer();
	void OnNewGame();
	void OnCampaign();
	void OnSaveGame();
	void OnSaveGameSelect(int result);
	void OnLoadGame();
	void OnLoadGameSelect(int result);

	void OnMultiPlayer();
	void OnHost();
	void OnJoin();
	void OnInternet();
	void OnNetworkProfile();

	void OnEditor();
	void OnNewMap();
	void OnMapSettings();
	void OnImportMap();
	void OnImportMapSelect(int result);
	void OnExportMap();
	void OnExportMapSelect(int result);

	void OnSettings();
	void OnExit();

//-------------------------------
	void OnCompny1();
	void OnCompny2();
	void OnCompny3();
	void OnCompny4();
	void OnCompny5();
	void OnCompny6();
//-------------------------------
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
	Window    *_panelFrame;
	Text      *_panelTitle;
	PanelType  _ptype;
	PanelState _pstate;
	GC_Menu *_AddOnButtons;
	GetFileNameDlg *_fileDlg;

	std::list<DelegateAdapter1<string_t> >  _campaigns;

public:
	MainMenuDlg(Window *parent);
	virtual ~MainMenuDlg();
	virtual void OnParentSize(float width, float height);
	virtual bool OnRawChar(int c);

protected:
	void OnTimeStep(float dt);
	void OnCloseChild(int result);
	void CreatePanel(); // create panel of current _ptype and go to PS_APPEARING state
	void SwitchPanel(PanelType newtype);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
