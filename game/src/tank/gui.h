// gui.h

#pragma once

#include "constants.h"
#include "config/Config.h"
#include <ui/Dialog.h>

class ConfVarTable;
class InputManager;
class AIManager;
class World;
class ThemeManager;
namespace FS
{
	class FileSystem;
}

namespace UI
{

// ui forward declarations
class Button;
class CheckBox;
class ComboBox;
class Edit;
class List;
class ListDataSourceMaps;
class ListDataSourceDefault;
class Text;
template<class, class> class ListAdapter;

class NewGameDlg : public UI::Dialog
{
	typedef ListAdapter<ListDataSourceMaps, List> MapList;
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;

	MapList      *_maps;
	DefaultListBox  *_players;
	DefaultListBox  *_bots;
	CheckBox  *_nightMode;
	Edit      *_gameSpeed;
	Edit      *_fragLimit;
	Edit      *_timeLimit;

	Button    *_removePlayer;
	Button    *_changePlayer;
	Button    *_removeBot;
	Button    *_changeBot;

	World &_world;
    InputManager &_inputMgr;
	AIManager &_aiMgr;
	const ThemeManager &_themeManager;
	FS::FileSystem &_fs;
	bool _newPlayer;

public:
	NewGameDlg(Window *parent, World &world, InputManager &inputMgr, AIManager &aiMgr, const ThemeManager &themeManager, FS::FileSystem &fs);
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

	std::vector<std::pair<std::string, std::string> > _classNames;

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

	std::vector<std::pair<std::string, std::string> > _classNames;

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
		const std::string &title,
		const std::string &text,
		const std::string &btn1,
		const std::string &btn2,
		const std::string &btn3
	);
	std::function<void(int)> eventSelect;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
