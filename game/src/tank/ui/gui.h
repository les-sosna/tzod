// gui.h

#pragma once

#include "gui_base.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////

class MainMenuDlg : public Dialog
{
	void OnNewGame();
	void OnExit();

public:
	MainMenuDlg(Window *parent);
	virtual void OnParentSize(float width, float height);
	virtual void OnRawChar(int c);
	virtual bool OnFocus(bool focus);

protected:
	void OnCloseChild(int result);
};

///////////////////////////////////////////////////////////////////////////////

class NewGameDlg : public Dialog
{
	List      *_maps;
	List      *_players;
	CheckBox  *_nightMode;
	Edit      *_gameSpeed;
	Edit      *_fragLimit;
	Edit      *_timeLimit;

	Button    *_removePlayer;
	Button    *_changePlayer;

public:
	NewGameDlg(Window *parent);

	virtual bool OnFocus(bool focus);

protected:
	void RefreshPlayersList();

protected:
	void OnAddPlayer();
	void OnRemovePlayer();
	void OnEditPlayer();

	void OnClosePlayerDlg(int result);

	void OnOK();
	void OnCancel();

	void OnSelectPlayer();
};

///////////////////////////////////////////////////////////////////////////////

class EditPlayerDlg : public Dialog
{
public:
	EditPlayerDlg(Window *parent);

protected:
	void OnOk();
	void OnCancel();
    
};

///////////////////////////////////////////////////////////////////////////////

class SkinSelectorDlg : public Dialog
{
public:
	SkinSelectorDlg(Window *parent);

protected:
	void OnOk();
	void OnCancel();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
