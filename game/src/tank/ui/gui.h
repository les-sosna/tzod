// gui.h

#pragma once

#include "Base.h"
#include "Dialog.h"


// forward declarations
class ConfVarTable;


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
	List      *_bots;
	CheckBox  *_nightMode;
	Edit      *_gameSpeed;
	Edit      *_fragLimit;
	Edit      *_timeLimit;

	Button    *_removePlayer;
	Button    *_changePlayer;
	Button    *_removeBot;
	Button    *_changeBot;

	bool       _newPlayer;

public:
	NewGameDlg(Window *parent);

	virtual void OnRawChar(int c);

protected:
	void RefreshPlayersList();
	void RefreshBotsList();

protected:
	void OnAddPlayer();
	void OnAddPlayerClose(int result);
	void OnEditPlayer();
	void OnEditPlayerClose(int result);
	void OnRemovePlayer();
	void OnSelectPlayer(int index);

	void OnAddBot();
	void OnAddBotClose(int result);
	void OnEditBot();
	void OnEditBotClose(int result);
	void OnRemoveBot();
	void OnSelectBot(int index);

	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////

class EditPlayerDlg : public Dialog
{
	Edit     *_name;
	ComboBox *_profiles;
	ComboBox *_skins;
	ComboBox *_classes;
	ComboBox *_teams;

	Window   *_skinPreview;

	std::vector<std::pair<string_t, string_t> > _classNames;

	ConfVarTable *_info;

public:
	EditPlayerDlg(Window *parent, ConfVarTable *info);

protected:
	void OnOK();
	void OnCancel();

	void OnChangeSkin(int index);
};

///////////////////////////////////////////////////////////////////////////////

class EditBotDlg : public Dialog
{
	Edit     *_name;
	ComboBox *_skins;
	ComboBox *_classes;
	ComboBox *_teams;
	ComboBox *_levels;

	Window   *_skinPreview;

	std::vector<std::pair<string_t, string_t> > _classNames;

	ConfVarTable *_info;

public:
	EditBotDlg(Window *parent, ConfVarTable *info);

protected:
	void OnOK();
	void OnCancel();

	void OnChangeSkin(int index);
};

///////////////////////////////////////////////////////////////////////////////

class SkinSelectorDlg : public Dialog
{
public:
	SkinSelectorDlg(Window *parent);

protected:
	void OnOK();
	void OnCancel();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
