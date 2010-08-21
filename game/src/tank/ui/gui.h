// gui.h

#pragma once

#include "Base.h"
#include "Dialog.h"

#include "config/Config.h"

// forward declarations
class ConfVarTable;


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

// forward declarations
class ListDataSourceMaps;

class NewGameDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceMaps, List> MapList;
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;

	MapList      *_maps;
	DefaultListBox  *_players;
	DefaultListBox  *_bots;
	CheckBox  *_nightMode;
	CheckBox  *_unlimmapMode;
	Edit      *_gameSpeed;
	Edit      *_fragLimit;
	Edit      *_timeLimit;

	Button    *_removePlayer;
	Button    *_changePlayer;
	Button    *_removeBot;
	Button    *_changeBot;

	bool _newPlayer;

public:
	NewGameDlg(Window *parent);
	virtual ~NewGameDlg();

	virtual bool OnRawChar(int c);

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
	typedef ListAdapter<ListDataSourceDefault, ComboBox> DefaultComboBox;

	Window   *_skinPreview;
	Edit     *_name;
	DefaultComboBox *_profiles;
	DefaultComboBox *_skins;
	DefaultComboBox *_classes;
	DefaultComboBox *_teams;

	std::vector<std::pair<string_t, string_t> > _classNames;

	ConfPlayerLocal _info;

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
	typedef ListAdapter<ListDataSourceDefault, ComboBox> DefaultComboBox;

	Edit     *_name;
	Window   *_skinPreview;
	DefaultComboBox *_skins;
	DefaultComboBox *_classes;
	DefaultComboBox *_teams;
	DefaultComboBox *_levels;

	std::vector<std::pair<string_t, string_t> > _classNames;

	ConfPlayerAI _info;

public:
	EditBotDlg(Window *parent, ConfVarTable *info);

	static const char levels[AI_MAX_LEVEL+1][16];

protected:
	void OnOK();
	void OnCancel();

	void OnChangeSkin(int index);
};

///////////////////////////////////////////////////////////////////////////////

class ScriptMessageBox : public Window
{
	Text   *_text;
	Button *_button1;
	Button *_button2;
	Button *_button3;

	void OnButton1();
	void OnButton2();
	void OnButton3();

public:
	ScriptMessageBox(Window *parent,
		const string_t &title,
		const string_t &text,
		const string_t &btn1,
		const string_t &btn2,
		const string_t &btn3
	);
	Delegate<void(int)> eventSelect;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
