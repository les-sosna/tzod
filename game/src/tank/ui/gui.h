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

	virtual void OnRawChar(int c);

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
	ComboBox *_types;
	ComboBox *_skins;
	ComboBox *_classes;

	Window   *_skinPreview;

public:
	EditPlayerDlg(Window *parent);

protected:
	void OnOk();
	void OnCancel();

	void OnChangeSkin();
    
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
