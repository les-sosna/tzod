// gui.h

#pragma once

#include "Base.h"
#include "Dialog.h"

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
	void OnAddPlayerClose(int result);

	void OnEditPlayer();
	void OnEditPlayerClose(int result);

	void OnRemovePlayer();

	void OnOK();
	void OnCancel();

	void OnSelectPlayer(int index);
};

///////////////////////////////////////////////////////////////////////////////

class EditPlayerDlg : public Dialog
{
	Edit     *_name;
	ComboBox *_types;
	ComboBox *_skins;
	ComboBox *_classesCombo;

	Window   *_skinPreview;

	std::vector<std::pair<string_t, string_t> > _classesNames;

	PlayerDesc &_playerDesc;

public:
	EditPlayerDlg(Window *parent, PlayerDesc &inout_desc, DWORD disablePlayers);

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
