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
class MapList;

class CreateServerDlg : public Dialog
{
	MapList   *_maps;
	CheckBox  *_nightMode;
	Edit      *_gameSpeed;
	Edit      *_fragLimit;
	Edit      *_timeLimit;
	Edit      *_svFps;
//	Edit      *_svLatency;

	CheckBox  *_lobbyEnable;
	ComboBox  *_lobbyList;
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
	Button *_btnOK;
	Edit   *_name;
	List   *_status;
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
	Button *_btnRefresh;
	Button *_btnConnect;
	Edit   *_name;
	List   *_servers;
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
	List           *_players;
	List           *_bots;
	Console        *_chat;
	Button         *_btnOK;
	SafePtr<ConsoleBuffer>  _buf;

	static const size_t _maxPings = 5;
	std::vector<DWORD> _pings;

public:
	WaitingForPlayersDlg(Window *parent);
	virtual ~WaitingForPlayersDlg();

protected:
	void OnAddBotClick();
	void OnAddBotClose(int result);
	void OnOK();
	void OnCancel();
	void OnSendMessage(const char *msg);

	void OnError(const std::string &msg);
	void OnMessage(const std::string &msg);
	void OnPlayerReady(unsigned short id, bool ready);
	void OnPlayersUpdate();
	void OnAddBot(const string_t &nick, const string_t &skin, int team, int level);
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
