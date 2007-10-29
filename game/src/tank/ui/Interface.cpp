// interface.cpp
//

#include "stdafx.h"

#include "interface.h"
#include "SaveLoad.h"
#include "macros.h"
#include "functions.h"

#include "level.h"

#include "directx.h"

#include "config/Config.h"

#include "video/RenderOpenGL.h"
#include "video/RenderDirect3D.h"
#include "video/TextureManager.h"

#include "core/debug.h"
#include "core/Console.h"

#include "ui/GuiManager.h"
#include "network/TankClient.h"
#include "network/TankServer.h"

#include "fs/MapFile.h"

#include "gc/Player.h"
#include "gc/Sound.h"
#include "gc/GameClasses.h"
#include "gc/RigidBody.h"

#include "res/resource.h"


#define GET_DLG_ITEM_TEXT(hdlg, id, str)      \
{                                             \
	HWND item = GetDlgItem(hdlg, id);         \
	_ASSERT(item);                            \
	int len = 1+GetWindowTextLength(item);    \
	str.resize(len);                          \
	GetWindowText(item, &str[0], len);        \
	str.resize(len-1);                        \
}


////////////////////////////////////////////////////////////////////

#define GETCHECK(id) (BST_CHECKED == SendDlgItemMessage(hDlg, (id), BM_GETCHECK, 0, 0))
#define SETCHECK(id, value) SendDlgItemMessage(hDlg, (id), BM_SETCHECK, (value) ? BST_CHECKED:BST_UNCHECKED, 0)

/////////////////////////////////////////////////////////////////////

void OnMouse(UINT message, WPARAM wParam, LPARAM lParam)
{
	if( g_gui )
	{
		g_gui->ProcessMouse(
			(float) (short) LOWORD(lParam),
			(float) (short) HIWORD(lParam),
			(float) (short) HIWORD(wParam) / 120.0f,
			message);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch( message )
	{
	case WM_CUSTOMCLIENTMSG:
		_ASSERT(g_client);
		if( g_client )
		{
			_ASSERT(g_client);
			return g_client->Mirror(wParam, lParam);
		}
		break;
	case WM_ACTIVATE:
	{
		WORD wActive = LOWORD(wParam);
		WORD wMinimized = HIWORD(wParam);

		if( (wMinimized || (wActive == WA_INACTIVE)) && !g_env.minimized )
		{
			TRACE("main window deactivated\n");

			g_env.minimized = true;
		//	if( g_level && !g_level->_client )
		//		g_level->_timer.Stop();
		}
		else if( g_env.minimized )
		{
			TRACE("main window activated\n");

			g_env.minimized = false;
		//	if( g_level && !g_level->_client )
		//		g_level->_timer.Start();
		}

		g_env.envInputs.bLButtonState = false;
		g_env.envInputs.bRButtonState = false;
		g_env.envInputs.bMButtonState = false;

		return 0;
	} break;

	case WM_SIZE:
	case WM_MOVE:
		if( g_render )
		{
			g_render->OnResizeWnd();
			if( g_gui )
				g_gui->Resize((float) g_render->GetWidth(), (float) g_render->GetHeight());
		}
		break;

	case WM_SETCURSOR:
		SetCursor(NULL);
		break;

	case WM_LBUTTONDOWN:
		if( hWnd != GetCapture() ) SetCapture(hWnd);
		g_env.envInputs.bLButtonState = true;
		OnMouse(message, wParam, lParam);
		break;
	case WM_RBUTTONDOWN:
		if( hWnd != GetCapture() ) SetCapture(hWnd);
		g_env.envInputs.bRButtonState = true;
		OnMouse(message, wParam, lParam);
		break;
	case WM_MBUTTONDOWN:
		if( hWnd != GetCapture() ) SetCapture(hWnd);
		g_env.envInputs.bMButtonState = true;
		OnMouse(message, wParam, lParam);
		break;
	case WM_LBUTTONUP:
		g_env.envInputs.bLButtonState = false;
		if( !g_env.envInputs.bRButtonState &&
			!g_env.envInputs.bMButtonState ) ReleaseCapture();
		OnMouse(message, wParam, lParam);
		break;
	case WM_RBUTTONUP:
		g_env.envInputs.bRButtonState = false;
		if( !g_env.envInputs.bLButtonState &&
			!g_env.envInputs.bMButtonState ) ReleaseCapture();
		OnMouse(message, wParam, lParam);
		break;
	case WM_MBUTTONUP:
		g_env.envInputs.bMButtonState = false;
		if( !g_env.envInputs.bLButtonState &&
			!g_env.envInputs.bRButtonState ) ReleaseCapture();
		OnMouse(message, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		g_env.envInputs.mouse_x = (short) LOWORD(lParam);
		g_env.envInputs.mouse_y = (short) HIWORD(lParam);
		OnMouse(message, wParam, lParam);
		break;
	case WM_MOUSEWHEEL:
		POINT pt;
		pt.x = (short) LOWORD(lParam),
		pt.y = (short) HIWORD(lParam),
		ScreenToClient(hWnd, &pt);
		OnMouse(message, wParam, MAKELPARAM(pt.x, pt.y) );
		break;

	case WM_CHAR:
	case WM_KEYDOWN:
	case WM_KEYUP:
		g_gui->ProcessKeys(message, wParam);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK dlgMsgBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
		{
		case BN_CLICKED:
			EndDialog(hDlg, wmId);
			break;
		case BN_SETFOCUS:
			SendMessage(hDlg, DM_SETDEFID, wmId, 0);
			break;
		}

		break;

	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_TEXT, WM_SETTEXT, 0, lParam);
		return TRUE;
	}
    return FALSE;
}

int MessageBoxT(
	HWND hWnd,          // handle of owner window
	LPCTSTR lpText,     // address of text in message box
	UINT uType)         // style of message box
{
	LPCTSTR templ;

	if( MB_OKCANCEL & uType )
		templ = (LPCTSTR) IDD_MSGBOX_OKCANCEL;
	else if( MB_YESNO & uType )
		templ = (LPCTSTR) IDD_MSGBOX_YESNO;
	else
		templ = (LPCTSTR) IDD_MSGBOX_OK;

	return DialogBoxParam(g_hInstance, templ, hWnd, (DLGPROC) dlgMsgBox, (LPARAM) lpText);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool operator < (const DisplayMode &left, const DisplayMode &right)
{
	return memcmp(&left, &right, sizeof(DisplayMode)) < 0;
}

void uiDisplaySettings(HWND hDlg, BOOL bSaveAndValidate)
{
	static BOOL s_bUpdating = FALSE;
	if( s_bUpdating ) return;
	s_bUpdating = TRUE;

	if( bSaveAndValidate )
	{
		g_conf.r_fullscreen->Set( GETCHECK(IDC_FULLSCREEN) );
		int render = GETCHECK(IDC_RENDER_OPENGL) ? 0 : 1;
		if( render != g_conf.r_render->GetInt() )
		{
			SAFE_DELETE(g_render);
			g_conf.r_render->SetInt(render);
		}
	}
	else
	{
		SETCHECK(IDC_FULLSCREEN, g_conf.r_fullscreen->Get());
		SETCHECK(g_conf.r_render->GetInt() ? IDC_RENDER_DIRECT3D : IDC_RENDER_OPENGL, TRUE);
	}


	if( !g_render )
	{
		g_render = g_conf.r_render->GetInt() ? renderCreateDirect3D() : renderCreateOpenGL();
	}

	typedef std::map<DisplayMode, DWORD> ModeMap;

	ModeMap screen_res, screen_rate, screen_bpp;

	DWORD mcount = g_render->getModeCount();
    for( DWORD i = 0; i < mcount; ++i )
	{
		DisplayMode mode, tmp;
		g_render->getDisplayMode(i, &mode);

		// resolution
		memset(&tmp, -1, sizeof(DisplayMode));
		tmp.Width  = mode.Width;
		tmp.Height = mode.Height;
		if( !screen_res.count(tmp) ) screen_res[tmp]++; else screen_res[tmp] = 0;

		// bpp
		memset(&tmp, -1, sizeof(DisplayMode));
		tmp.BitsPerPixel = mode.BitsPerPixel;
		if( !screen_bpp.count(tmp) ) screen_bpp[tmp]++; else screen_bpp[tmp] = 0;

		// rate
		memset(&tmp, -1, sizeof(DisplayMode));
		tmp.RefreshRate = mode.RefreshRate;
		if( !screen_rate.count(tmp) ) screen_rate[tmp]++; else screen_rate[tmp] = 0;
	}


	for( int i = bSaveAndValidate?0:1; i < 2; ++i )
	{
		if( i )
		{
			SendDlgItemMessage(hDlg, IDC_BPP,        CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hDlg, IDC_RATE,       CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hDlg, IDC_RESOLUTION, CB_RESETCONTENT, 0, 0);
		}

		// resolution
		for( ModeMap::iterator it = screen_res.begin(); it != screen_res.end(); ++it )
		{
			char buf[256];
			if( it->first.Width < 800 || it->first.Height < 600 ) continue;
			wsprintf(buf, "%4d x %4d", it->first.Width, it->first.Height);
			if( i )
			{
				int index = SendDlgItemMessage(hDlg, IDC_RESOLUTION, CB_ADDSTRING, 0, (LPARAM) buf);
				if( it->first.Width == g_conf.r_width->GetInt() &&
					it->first.Height == g_conf.r_height->GetInt() )
				{
					SendDlgItemMessage(hDlg, IDC_RESOLUTION, CB_SETCURSEL, index, 0);
				}
			}
			else
			{
				char item_text[256] = {0};
				SendDlgItemMessage(hDlg, IDC_RESOLUTION, CB_GETLBTEXT,
					SendDlgItemMessage(hDlg,IDC_RESOLUTION,CB_GETCURSEL,0,0), (LPARAM)item_text);
				if( 0 == strcmp(item_text, buf) )
				{
					g_conf.r_width->SetInt( it->first.Width );
					g_conf.r_height->SetInt( it->first.Height );
				}
			}
		}
		if( -1 == SendDlgItemMessage(hDlg, IDC_RESOLUTION, CB_GETCURSEL, 0, 0) )
			SendDlgItemMessage(hDlg, IDC_RESOLUTION, CB_SETCURSEL, 0, 0);


		// color depth
		for( ModeMap::iterator it = screen_bpp.begin(); it != screen_bpp.end(); ++it )
		{
			char buf[256];
			if( it->first.BitsPerPixel < 16 ) continue;
			wsprintf(buf, "%d bit", it->first.BitsPerPixel);
			if( i )
			{
				int index = SendDlgItemMessage(hDlg, IDC_BPP, CB_ADDSTRING, 0, (LPARAM) buf);
				if( it->first.BitsPerPixel == g_conf.r_bpp->GetInt() )
					SendDlgItemMessage(hDlg, IDC_BPP, CB_SETCURSEL, index, 0);
			}
			else
			{
				char item_text[256] = {0};
				SendDlgItemMessage(hDlg, IDC_BPP, CB_GETLBTEXT,
					SendDlgItemMessage(hDlg,IDC_BPP,CB_GETCURSEL,0,0), (LPARAM)item_text);
				if( 0 == strcmp(item_text, buf) )
				{
					g_conf.r_bpp->SetInt(it->first.BitsPerPixel);
				}
			}
		}
		if( -1 == SendDlgItemMessage(hDlg, IDC_BPP, CB_GETCURSEL, 0, 0) )
			SendDlgItemMessage(hDlg, IDC_BPP, CB_SETCURSEL, 0, 0);


		// refresh rate
		for( ModeMap::iterator it = screen_rate.begin(); it != screen_rate.end(); ++it )
		{
			char buf[256];
			wsprintf(buf, "%d Hz", it->first.RefreshRate);
			if( i )
			{
				int index = SendDlgItemMessage(hDlg, IDC_RATE, CB_ADDSTRING, 0, (LPARAM) buf);
				if( it->first.RefreshRate == g_conf.r_freq->GetInt() )
					SendDlgItemMessage(hDlg, IDC_RATE, CB_SETCURSEL, index, 0);
			}
			else
			{
				char item_text[256] = {0};
				SendDlgItemMessage(hDlg, IDC_RATE, CB_GETLBTEXT,
					SendDlgItemMessage(hDlg,IDC_RATE,CB_GETCURSEL,0,0), (LPARAM)item_text);
				if( 0 == strcmp(item_text, buf) )
				{
					g_conf.r_freq->SetInt(it->first.RefreshRate);
				}
			}
		}
		if( -1 == SendDlgItemMessage(hDlg, IDC_RATE, CB_GETCURSEL, 0, 0) )
			SendDlgItemMessage(hDlg, IDC_RATE, CB_SETCURSEL, 0, 0);
	}

	EnableWindow(GetDlgItem(hDlg, IDC_BPP ), g_conf.r_fullscreen->Get() );
	EnableWindow(GetDlgItem(hDlg, IDC_RATE), g_conf.r_fullscreen->Get() );

	s_bUpdating = FALSE;
}

LRESULT CALLBACK dlgDisplaySettings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
		{
		case BN_CLICKED:
			switch (wmId)
			{
			case IDC_RENDER_OPENGL:
			case IDC_RENDER_DIRECT3D:
			case IDC_FULLSCREEN:
				uiDisplaySettings(hDlg, TRUE);
				break;
			case IDOK:
				uiDisplaySettings(hDlg, TRUE);
				EndDialog(hDlg, wmId);
				break;
			case IDCANCEL:
				EndDialog(hDlg, wmId);
				break;
			}
			break;
		}
		break;

	case WM_INITDIALOG:
		uiDisplaySettings(hDlg, FALSE);
		return TRUE;
	}
	return FALSE;
}



/*

LRESULT CALLBACK dlgNetPlayers(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HFONT hFontTitle;
	static HFONT hFontChat;

	int wmId, wmEvent;
	switch (message)
	{
	case WM_NEWDATA:
	{
		DataBlock db;
		while( g_level->_client->GetData(db) )
		{
			switch( db.type() )
			{
			case DBTYPE_PING:
				OutputTextBox(GetDlgItem(hDlg, IDC_CHAT), "ваш пинг %d ms\n",
					timeGetTime() - db.cast<DWORD>());
				break;
			case DBTYPE_PLAYERREADY:
			{
				HWND hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);
				int count = g_level->players.size();
				_ASSERT(ListView_GetItemCount(hwndLV) == count);

				LVITEM lvi = {0};
				lvi.mask     = LVIF_PARAM;
				lvi.iSubItem = 0;

				const DWORD who = db.cast<dbPlayerReady>().player_id;

				int index = 0;
				for( ;index < count; ++index )
				{
					lvi.iItem = index;

					BOOL res = ListView_GetItem(hwndLV, &lvi);
					_ASSERT(res);
					_ASSERT(lvi.lParam);

					GC_Player *pPlayer = (GC_Player *) lvi.lParam;
					_ASSERT(!pPlayer->IsKilled());
					_ASSERT(0 != pPlayer->_networkId);

					if( who == pPlayer->_networkId )
					{
						if( db.cast<dbPlayerReady>().ready )
						{
							ListView_SetItemText(hwndLV, index, 3, "Готов");
						//	OutputTextBox(GetDlgItem(hDlg, IDC_CHAT), "'%s' готов!\n", pPlayer->_name);
						}
						else
						{
							ListView_SetItemText(hwndLV, index, 3, "");
						}
						break;
					}
				}
				_ASSERT(index < count);
			} break;
			case DBTYPE_PLAYERQUIT:
			{
				HWND hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);
				int count = g_level->players.size();
				_ASSERT(ListView_GetItemCount(hwndLV) == count);

				LVITEM lvi = {0};
				lvi.mask     = LVIF_PARAM;
				lvi.iSubItem = 0;

				const DWORD who = db.cast<DWORD>();

				int index = 0;
				for( ; index < count; ++index )
				{
					lvi.iItem = index;

					BOOL res = ListView_GetItem(hwndLV, &lvi);
					_ASSERT(res);
					_ASSERT(lvi.lParam);

					GC_Player *pPlayer = (GC_Player *) lvi.lParam;
					_ASSERT(!pPlayer->IsKilled());
					_ASSERT(0 != pPlayer->_networkId);

					if( who == pPlayer->_networkId )
					{
						ListView_DeleteItem(hwndLV, index);
						if( 0 == ListView_GetItemCount(hwndLV) )
							EnableWindow(hwndLV, FALSE);

						OutputTextBox(GetDlgItem(hDlg, IDC_CHAT), "%s покинул игру.\n", pPlayer->_name);
						pPlayer->Kill();
						break;
					}
				}
				_ASSERT(index < count);
			} break;
			case DBTYPE_NEWPLAYER:
			{
				PLAYERDESCEX &pd = db.cast<PLAYERDESCEX>();

				GC_Player *pPlayer = new GC_Player(0);
				strcpy(pPlayer->_name, pd.name);
				strcpy(pPlayer->_skin, pd.skin);
				pPlayer->_team        = pd.team;
				pPlayer->_networkId = pd.dwNetworkId;
				pPlayer->UpdateSkin();
				pPlayer->SetController( pd.type );

				HWND hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);
				AddPlayerToListView(hwndLV, pPlayer->_name, pPlayer->_skin, pPlayer->_team, pPlayer);
				if( pd.type >= MAX_HUMANS )
					ListView_SetItemText(hwndLV, ListView_GetItemCount(hwndLV) - 1, 3, "Бот");

				EnableWindow(hwndLV, TRUE);

				if( g_level->_client->GetId() )
					EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);

				OutputTextBox(GetDlgItem(hDlg, IDC_CHAT), "%s вошел в игру.\n", pPlayer->_name);
			} break;
			case DBTYPE_ERRORMSG:
				EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
			case DBTYPE_TEXTMESSAGE:
				OutputTextBox(GetDlgItem(hDlg, IDC_CHAT), "%s\n", (char*) db.data());
				break;
			case DBTYPE_STARTGAME:
			{
				OPT(bAllowEditor)        = FALSE;
				OPT(bAllowPlayerChange)  = FALSE;

				PUSH(GetParent(hDlg), IDCANCEL);
				EndDialog(hDlg, 0);

				g_level->_client->SetWindow(g_env.hMainWnd);
				g_level->_timer.Start();

				for( size_t i = 0; i < g_level->_client->_latency; ++i )
					g_level->_client->SendControl(ControlPacket());
			} break;
			default:
				_ASSERT(FALSE);
			} // end of switch( db.type() )
		} // end of while

		if( !g_level ) break;
	} break; // end of case WM_NEWDATA
	case WM_NOTIFY:
	{
		wmId = (int) wParam;
		NMHDR *phdr = (LPNMHDR) lParam;
		switch (phdr->code)
		{
		case LVN_KEYDOWN:
			switch (((NMLVKEYDOWN *) lParam)->wVKey)
			{
			case VK_DELETE:
				PUSH(hDlg, IDC_KICK);
				break;
			case VK_INSERT:
				PUSH(hDlg, IDC_ADD);
				break;
			}
			break;
		case LVN_ITEMACTIVATE:
			PUSH(hDlg, IDC_EDIT);
			break;
		case LVN_ITEMCHANGED:
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT), ListView_GetSelectedCount(phdr->hwndFrom));
			EnableWindow(GetDlgItem(hDlg, IDC_KICK), ListView_GetSelectedCount(phdr->hwndFrom));
			break;
		}
	} break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
		{
		case EN_SETFOCUS:
			if( IDC_INPUT == wmId )
				SendMessage(hDlg, DM_SETDEFID, IDC_SEND, 0);
			break;
		case EN_CHANGE:
			if( IDC_INPUT == wmId )
			{
				EnableWindow( GetDlgItem(hDlg, IDC_SEND),
					0 != GetWindowTextLength(GetDlgItem(hDlg, IDC_INPUT)) );
			}
			break;
		case BN_SETFOCUS:
			SendMessage(hDlg, DM_SETDEFID, wmId, 0);
			break;
		case BN_CLICKED:
			switch( wmId )
			{
			case IDC_SEND:
			{
				char buf[MAX_PATH+1];
				GetDlgItemText(hDlg, IDC_INPUT, buf, MAX_PATH);
				size_t l = strlen(buf);
				if( 0 != l )
				{
					if( 0 == strcmp(buf, "/ping") )
					{
						DataBlock db(sizeof(DWORD));
						db.type() = DBTYPE_PING;
						db.cast<DWORD>() = timeGetTime();
						g_level->_client->SendDataToServer(db);
						db.cast<DWORD>() = timeGetTime();
						g_level->_client->SendDataToServer(db);
						db.cast<DWORD>() = timeGetTime();
						g_level->_client->SendDataToServer(db);
					}
					else
					{
						DataBlock db(l + 1);
						strcpy((char*) db.data(), buf);
						db.type() = DBTYPE_TEXTMESSAGE;
						g_level->_client->SendDataToServer(db);
					}
					SetDlgItemText(hDlg, IDC_INPUT, "");
				}
			} break;
			case IDC_ADD:
			{
				PLAYERDESCEX pde;
				ZeroMemory(&pde, sizeof(PLAYERDESCEX));

				std::vector<string_t> names;
				g_texman->GetTextureNames(names, "skin/");
				strcpy(pde.skin, names[rand() % names.size()].c_str());

				strcpy(pde.name, "new bot");
				pde.dwHasPlayers = 0xFFFFFFFF;
				pde.type = -1;

				LOGOUT_1("DialogBox(IDD_ADDPLAYER)\n");
				if (IDOK == DialogBoxParam(g_hInstance, (LPCTSTR)IDD_ADDPLAYER, hDlg,
					(DLGPROC) dlgAddPlayer, (LPARAM) &pde) )
				{
					DataBlock db(sizeof(PLAYERDESC));
					db.type() = DBTYPE_NEWPLAYER;
					memcpy(db.data(), &pde, sizeof(PLAYERDESC));
					g_level->_client->SendDataToServer(db);
				}
			} break;
//			case IDC_KICK:
//			{
//				HWND hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);
//				int index = ListView_GetSelectionMark(hwndLV);
//				if (-1 != index && IDYES == MessageBoxT(hDlg, "Удалить игрока?", MB_YESNO|MB_ICONQUESTION))
//				{
//					LVITEM lvi = {0};
//					lvi.iItem = index;
//					lvi.iSubItem = 0;
//					lvi.mask = LVIF_PARAM;
//
//					ListView_GetItem(hwndLV, &lvi);
//					_ASSERT(lvi.lParam);
//
//					((GC_Player *) lvi.lParam)->Kill();
//					ListView_DeleteItem(hwndLV, index);
//
//					if (0 == ListView_GetItemCount(hwndLV))
//					{
//						EnableWindow(hwndLV, FALSE);
//						SetFocus(GetDlgItem(hDlg, IDC_ADD));
//					}
//				}
//			} break;
			case IDOK:
			{
				DataBlock db(sizeof(dbPlayerReady));
				db.type() = DBTYPE_PLAYERREADY;
				db.cast<dbPlayerReady>().player_id = g_level->_client->GetId();
				db.cast<dbPlayerReady>().ready     = TRUE;
				g_level->_client->SendDataToServer(db);
			} break;
			case IDCANCEL:
				SAFE_DELETE(g_level);
				EndDialog(hDlg, wmId);
				break;
			}
			break;
		} // end of switch( wmId )
		break;

	case WM_INITDIALOG:
	{
		//
		// установка шрифтов
		//

		LOGFONT lf   = {0};
		lf.lfCharSet = RUSSIAN_CHARSET;
		strcpy(lf.lfFaceName, "Fixedsys");
		hFontChat = CreateFontIndirect(&lf);
		SendDlgItemMessage(hDlg, IDC_CHAT, WM_SETFONT, (WPARAM) hFontChat, 1);
		SendDlgItemMessage(hDlg, IDC_INPUT, WM_SETFONT, (WPARAM) hFontChat, 1);

		lf.lfHeight  = 28;
		lf.lfWeight  = FW_BLACK;
		strcpy(lf.lfFaceName, "Arial");
		hFontTitle = CreateFontIndirect(&lf);
		SendDlgItemMessage(hDlg, IDC_TITLE, WM_SETFONT, (WPARAM) hFontTitle, 1);

		SetDlgItemText(hDlg, IDC_TITLE, OPT(cServerName) );


		//
		// настройка списка игроков
		//

		HWND hwndLV;
		hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);

		ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);
		ListView_SetBkColor(hwndLV, ICOLOR_BACKGROUND);

		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;

		lvc.cx = 170;
		lvc.iSubItem = 0;
		lvc.pszText = "Имя";
		ListView_InsertColumn(hwndLV, 0, &lvc);

		lvc.cx = 80;
		lvc.iSubItem++;
		lvc.pszText = "Скин";
		ListView_InsertColumn(hwndLV, 1, &lvc);

		lvc.cx = 80;
		lvc.iSubItem++;
		lvc.pszText = "Команда";
		ListView_InsertColumn(hwndLV, 3, &lvc);

		lvc.cx = 80;
		lvc.iSubItem++;
		lvc.pszText = "Статус";
		ListView_InsertColumn(hwndLV, 3, &lvc);


		EnableWindow(hwndLV, 0 != ListView_GetItemCount(hwndLV));
		EnableWindow(GetDlgItem(hDlg, IDC_SEND), FALSE);

		SetFocus(GetDlgItem(hDlg, IDOK));

		//////////////

		g_level->_client->SetWindow(hDlg);

		// отправка информации об игроке
		g_level->_client->SendDataToServer(
			DataWrap(OPT(pdeLocalPlayer), DBTYPE_NEWPLAYER));

	} return TRUE;
	case WM_DESTROY:
		SendDlgItemMessage(hDlg, IDC_TITLE, WM_SETFONT, NULL, 1);
		SendDlgItemMessage(hDlg, IDC_CHAT, WM_SETFONT, NULL, 1);
		SendDlgItemMessage(hDlg, IDC_INPUT, WM_SETFONT, NULL, 1);
		DeleteObject(hFontTitle);
		DeleteObject(hFontChat);
		break;
	}
    return FALSE;
}

*/


///////////////////////////////////////////////////////////////////////////////
// end of file
