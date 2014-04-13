// gui_network.h

#pragma once

#include "Dialog.h"
#include "ClientBase.h"

namespace UI
{
// forward declarations
class List;
class ListDataSourceMaps;
class ListDataSourceDefault;
class Edit;
class ComboBox;
class CheckBox;
class Button;
class Console;
class ConsoleBuffer;
template <class, class> class ListAdapter;

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

class ConnectDlg
	: public Dialog
	, private IClientCallback
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_status;
	Button *_btnOK;
	Edit   *_name;
	std::unique_ptr<Subscribtion> _clientSubscribtion;

public:
	ConnectDlg(Window *parent, const std::string &defaultName);
	virtual ~ConnectDlg();

protected:
	void OnOK();
	void OnCancel();

private:
	// IClientCallback
	virtual void OnConnected();
	virtual void OnErrorMessage(const std::string &msg);
	virtual void OnTextMessage(const std::string &msg);
	virtual void OnClientDestroy();
};

///////////////////////////////////////////////////////////////////////////////

//class LobbyClient;
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

//	SafePtr<LobbyClient> _client;
};

///////////////////////////////////////////////////////////////////////////////

class WaitingForPlayersDlg
	: public Dialog
	, private IClientCallback
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_players;
	DefaultListBox *_bots;
	Console        *_chat;
	Button         *_btnOK;
	Button         *_btnProfile;
	std::unique_ptr<UI::ConsoleBuffer>  _buf;
	std::unique_ptr<Subscribtion> _clientSubscribtion;

	static const size_t _maxPings = 5;
	std::vector<unsigned int> _pings;

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
	void OnSendMessage(const std::string &msg);

private:
	// IClientCallback
	virtual void OnErrorMessage(const std::string &msg);
	virtual void OnTextMessage(const std::string &msg);
	virtual void OnPlayerReady(size_t playerIdx, bool ready);
	virtual void OnPlayersUpdate();
	virtual void OnStartGame();
	virtual void OnClientDestroy();
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
