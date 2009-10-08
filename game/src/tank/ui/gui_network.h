// gui_network.h

#pragma once

#include "Base.h"
#include "Dialog.h"

// forward declarations
class LobbyClient;

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

// forward declarations
class ListDataSourceMaps;

class CreateServerDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceMaps, List> MapListBox;
	typedef ListAdapter<ListDataSourceDefault, ComboBox> DefaultComboBox;

	MapListBox *_maps;
	CheckBox  *_nightMode;
	Edit      *_gameSpeed;
	Edit      *_fragLimit;
	Edit      *_timeLimit;
	Edit      *_svFps;
//	Edit      *_svLatency;

	DefaultComboBox *_lobbyList;
	CheckBox  *_lobbyEnable;
	Button    *_lobbyAdd;

public:
	CreateServerDlg(Window *parent);
	virtual ~CreateServerDlg();

protected:
	void OnOK();
	void OnCancel();
	void OnLobbyEnable();
	virtual void OnCloseChild(int result);
};

///////////////////////////////////////////////////////////////////////////////

class ConnectDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_status;
	Button *_btnOK;
	Edit   *_name;
	bool    _auto;

public:
	ConnectDlg(Window *parent, const char *autoConnect);
	virtual ~ConnectDlg();

protected:
	void OnOK();
	void OnCancel();

	void OnConnected();
	void OnError(const std::string &msg);
	void OnMessage(const std::string &msg);
};

///////////////////////////////////////////////////////////////////////////////

class InternetDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_servers;
	Button *_btnRefresh;
	Button *_btnConnect;
	Edit   *_name;
	Text   *_status;

public:
	InternetDlg(Window *parent);
	virtual ~InternetDlg();

protected:
	void OnRefresh();
	void OnConnect();
	void OnCancel();
	void OnSelectServer(int idx);

	void OnCloseChild(int result);

	void OnLobbyError(const std::string &msg);
	void OnLobbyList(const std::vector<std::string> &result);

	void Error(const char *msg);

	SafePtr<LobbyClient> _client;
};

///////////////////////////////////////////////////////////////////////////////

class WaitingForPlayersDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_players;
	DefaultListBox *_bots;
	Console        *_chat;
	Button         *_btnOK;
	Button         *_btnProfile;
	std::auto_ptr<UI::ConsoleBuffer>  _buf;

	static const size_t _maxPings = 5;
	std::vector<DWORD> _pings;

public:
	WaitingForPlayersDlg(Window *parent);
	virtual ~WaitingForPlayersDlg();

protected:
	// GUI event handlers
	void OnCloseProfileDlg(int result);
	void OnChangeProfileClick();
	void OnAddBotClick();
	void OnAddBotClose(int result);
	void OnOK();
	void OnCancel();
	void OnSendMessage(const string_t &msg);

	// client event handlers
	void OnError(const std::string &msg);
	void OnMessage(const std::string &msg);
	void OnPlayerReady(unsigned short id, bool ready);
	void OnPlayersUpdate();
	void OnStartGame();
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
