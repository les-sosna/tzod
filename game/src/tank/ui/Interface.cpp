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
	switch (message)
	{
	case WM_CUSTOMCLIENTMSG:
		if( g_level )
		{
			_ASSERT(g_level->_client);
			return g_level->_client->Mirror(wParam, lParam);
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
		SAFE_DELETE(g_level);
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

LRESULT CALLBACK dlgNewMap(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y;
	int wmId, wmEvent;
	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch( wmEvent )
		{
		case EN_CHANGE:
			switch (wmId)
			{
			case IDC_X:
			case IDC_Y:
				break;
			}
			break;
		case BN_SETFOCUS:
			SendMessage(hDlg, DM_SETDEFID, wmId, 0);
			break;
		case BN_KILLFOCUS:
			SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
			break;
		case EN_KILLFOCUS:
			switch (wmId)
			{
			case IDC_X:
				x = __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, GetDlgItemInt(hDlg, IDC_X, NULL, FALSE)));
				if( GetDlgItemInt(hDlg, IDC_X, NULL, FALSE) != x )
					SetDlgItemInt(hDlg, IDC_X, x, FALSE);
				break;
			case IDC_Y:
				y = __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, GetDlgItemInt(hDlg, IDC_Y, NULL, FALSE)));
				if( GetDlgItemInt(hDlg, IDC_Y, NULL, FALSE) != y )
					SetDlgItemInt(hDlg, IDC_Y, y, FALSE);
				break;
			}
			break;
		case BN_CLICKED:
			switch (wmId)
			{
			case IDOK:
				g_conf.ed_width->SetInt( __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, GetDlgItemInt(hDlg, IDC_X, NULL, FALSE))) );
				g_conf.ed_height->SetInt( __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, GetDlgItemInt(hDlg, IDC_Y, NULL, FALSE))) );
			case IDCANCEL:
				EndDialog(hDlg, wmId);
				break;
			}
			break;
		}
		break;
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_X_SPIN, UDM_SETRANGE, 0, MAKELONG(LEVEL_MAXSIZE, LEVEL_MINSIZE));
		SendDlgItemMessage(hDlg, IDC_Y_SPIN, UDM_SETRANGE, 0, MAKELONG(LEVEL_MAXSIZE, LEVEL_MINSIZE));
		SetDlgItemInt(hDlg, IDC_X, __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, g_conf.ed_width->GetInt() )), FALSE);
		SetDlgItemInt(hDlg, IDC_Y, __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, g_conf.ed_height->GetInt() )), FALSE);
		return TRUE;
	}
    return FALSE;
}

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


LRESULT CALLBACK dlgOptions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	static HFONT hFont;
	static LONG  lInitVolume; // в случае отмены восстанавливаем значение громкости

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
		{
		case BN_KILLFOCUS:
			SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
			break;
		case BN_SETFOCUS:
			switch( wmId )
			{
			case IDOK:
			case IDCANCEL:
			case IDC_MORE:
				SendMessage(hDlg, DM_SETDEFID, wmId, 0);
				break;
			default:
				SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
			}
			break;
		case BN_CLICKED:
			switch (wmId)
			{
			case IDOK:
				g_conf.g_particles->Set(  GETCHECK(IDC_CHK_PARTICLES) );
				g_conf.g_showdamage->Set( GETCHECK(IDC_CHK_DAMLABEL)  );
				g_conf.ui_showfps->Set(   GETCHECK(IDC_CHK_FPS)       );
				g_conf.ui_showtime->Set(  GETCHECK(IDC_CHK_TIMER)     );
				g_conf.r_askformode->Set( GETCHECK(IDC_CHK_SHOWSELECTMODE) );
				//-------------------------------
				g_conf.s_maxchanels->SetInt(GetDlgItemInt(hDlg, IDC_MAXSOUNDS, NULL, FALSE));
				if( 0 == g_conf.s_maxchanels->GetInt() )
					g_conf.s_maxchanels->SetInt(0);
				//-------------------------------
				EndDialog(hDlg, wmId);
				break;
			case IDCANCEL:
				g_conf.s_volume->SetInt(lInitVolume);
				EndDialog(hDlg, wmId);
				break;
			//case IDC_SETTINGS_PLAYER1:
			//case IDC_SETTINGS_PLAYER2:
			//case IDC_SETTINGS_PLAYER3:
			//case IDC_SETTINGS_PLAYER4:
			//{
			//	int index = wmId - IDC_SETTINGS_PLAYER1;

			//	DialogBoxParam(g_hInstance, (LPCTSTR)IDD_DIALOG_CONTROLS, hDlg,
			//		(DLGPROC) dlgControls, (LPARAM) &g_options.players[index]);

			//	FreeDirectInput();
			//	InitDirectInput(g_env.hMainWnd);

			//	if( g_level )
			//	{
			//		FOREACH( players, GC_Player, pPlayer )
			//		{
			//			if( pPlayer->_nIndex == index ) pPlayer->SetController(index);
			//		}
			//	}
			//} break;
			case IDC_MORE:
			{
				const int newheight = 400;
				RECT rt, br;
				GetClientRect(hDlg, &rt);
				SetWindowPos(hDlg, NULL, 0, 0, rt.right - rt.left, newheight,
					SWP_NOZORDER | SWP_NOMOVE);
				rt.top = rt.bottom - 8;
				InvalidateRect(hDlg, &rt, TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_MORE), FALSE);
				///////////////////////////////////////
				GetWindowRect(GetDlgItem(hDlg, IDOK), &br);
				ScreenToClient(hDlg, (LPPOINT) &br);
				ScreenToClient(hDlg, (LPPOINT) &br + 1);
				OffsetRect(&br, 0, newheight - rt.bottom);
				MoveWindow(GetDlgItem(hDlg, IDOK), br.left, br.top,
					br.right-br.left, br.bottom-br.top, TRUE);
				///////////////////////////////////////
				GetWindowRect(GetDlgItem(hDlg, IDCANCEL), &br);
				ScreenToClient(hDlg, (LPPOINT) &br);
				ScreenToClient(hDlg, (LPPOINT) &br + 1);
				OffsetRect(&br, 0, newheight - rt.bottom);
				MoveWindow(GetDlgItem(hDlg, IDCANCEL), br.left, br.top,
					br.right-br.left, br.bottom-br.top, TRUE);
				///////////////////////////////////////
				EnableWindow(GetDlgItem(hDlg, IDC_MAXSOUNDS), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_RATE), TRUE);
				SetFocus(GetDlgItem(hDlg, IDC_MAXSOUNDS));
			} break;
			} // end of switch (wmId)
			break;
		} // end of switch (wmEvent)
		break;

	case WM_VSCROLL:
	{
		LONG lVolume = 101 - SendDlgItemMessage(hDlg, IDC_VOLUME, TBM_GETPOS, 0, 0);
		char str[8];
		wsprintf(str, "%d%%", lVolume);
		SetDlgItemText(hDlg, IDC_INDICATOR, str);
		g_conf.s_volume->SetInt(int(2171.0f * logf((float) lVolume * 0.01f)));
	} break;

	case WM_INITDIALOG:
	{
		//
		// установка крупного шрифта в заголовке
		//
		LOGFONT lf = {0};
		lf.lfHeight    = 28;
		lf.lfWeight    = FW_BLACK;
		lf.lfCharSet = RUSSIAN_CHARSET;
		strcpy(lf.lfFaceName, "Arial");
		hFont = CreateFontIndirect(&lf);
		SendDlgItemMessage(hDlg, IDC_TITLE, WM_SETFONT, (WPARAM) hFont, 1);

		//
		// установка опций
		//

		// основные

		SETCHECK( IDC_CHK_PARTICLES, g_conf.g_particles->Get()  );
		SETCHECK( IDC_CHK_DAMLABEL,  g_conf.g_showdamage->Get() );
		SETCHECK( IDC_CHK_FPS,       g_conf.ui_showfps->Get()   );
		SETCHECK( IDC_CHK_TIMER,     g_conf.ui_showtime->Get()  );
		SETCHECK( IDC_CHK_SHOWSELECTMODE, g_conf.r_askformode->Get() );

		lInitVolume = g_conf.s_volume->GetInt();
		SendDlgItemMessage(hDlg, IDC_VOLUME, TBM_SETRANGE, (WPARAM) FALSE, (LPARAM) MAKELONG(1, 100));
		SendDlgItemMessage(hDlg, IDC_VOLUME, TBM_SETPOS,   (WPARAM) TRUE,  (LPARAM) (101 - int(100.0f * expf((float)lInitVolume / 2171.0f))));
		SendMessage(hDlg, WM_VSCROLL, 0, 0);

		// дополнительные

		SendDlgItemMessage(hDlg, IDC_MAXSOUNDS, CB_INSERTSTRING, 0, (LPARAM) "8");
		SendDlgItemMessage(hDlg, IDC_MAXSOUNDS, CB_INSERTSTRING, 1, (LPARAM) "16");
		SendDlgItemMessage(hDlg, IDC_MAXSOUNDS, CB_INSERTSTRING, 2, (LPARAM) "32");
		SendDlgItemMessage(hDlg, IDC_MAXSOUNDS, CB_INSERTSTRING, 3, (LPARAM) "Не ограничивать");
		for( int i = 0; i < 4; ++i )
		{
			static int schan[4] = {8, 16, 32, 0xFFFF};
			if( schan[i] == g_conf.s_maxchanels->GetInt() )
			{
				SendDlgItemMessage(hDlg, IDC_MAXSOUNDS, CB_SETCURSEL, i, 0);
				break;
			}
		}


		//
		// спрятать дополнительные опции
		//

		RECT rt;
		GetClientRect(hDlg, &rt);
		SetWindowPos(hDlg, NULL, 0, 0, rt.right - rt.left, 256, SWP_NOZORDER | SWP_NOMOVE);
	} return TRUE;
	case WM_DESTROY:
		DeleteObject(hFont);
		break;
	} // end of switch (message)
    return FALSE;
}


LRESULT CALLBACK dlgMapSettings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (message)
	{
		case WM_COMMAND:
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			switch (wmId)
			{
			case IDC_DESC:
				InvalidateRect((HWND) lParam, NULL, FALSE);
				break;
			case IDOK:
			{
				GET_DLG_ITEM_TEXT(hDlg, IDC_AUTHOR, g_level->_infoAuthor);
				GET_DLG_ITEM_TEXT(hDlg, IDC_EMAIL,  g_level->_infoEmail);
				GET_DLG_ITEM_TEXT(hDlg, IDC_URL,    g_level->_infoUrl);
				GET_DLG_ITEM_TEXT(hDlg, IDC_DESC,   g_level->_infoDesc);

				int i = SendDlgItemMessage(hDlg, IDC_THEME, CB_GETCURSEL, 0, 0);
				if( 0 != i )
				{
					g_level->_infoTheme.resize(
						SendDlgItemMessage(hDlg, IDC_THEME, CB_GETLBTEXTLEN, i, 0)
					);
					SendDlgItemMessage( hDlg, IDC_THEME, CB_GETLBTEXT, i,
						(LPARAM) &*g_level->_infoTheme.begin() );
				}
				else
				{
					g_level->_infoTheme.clear();
				}
				if( !_ThemeManager::Inst().ApplyTheme(i) )
				{
					MessageBoxT(hDlg, "Ошибка при загрузке темы", MB_ICONERROR);
				}
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			} break;

			case IDCANCEL:
				EndDialog(hDlg, LOWORD(wParam));
				return FALSE;
				break;
			}
			break;


		case WM_INITDIALOG:
		{
			SetDlgItemText(hDlg, IDC_AUTHOR, g_level->_infoAuthor.c_str());
			SetDlgItemText(hDlg, IDC_EMAIL,  g_level->_infoEmail.c_str());
			SetDlgItemText(hDlg, IDC_URL,    g_level->_infoUrl.c_str());
			SetDlgItemText(hDlg, IDC_DESC,   g_level->_infoDesc.c_str());

			for( size_t i = 0; i < _ThemeManager::Inst().GetThemeCount(); i++ )
			{
				SendDlgItemMessage(hDlg, IDC_THEME, CB_ADDSTRING, 0,
					(LPARAM) _ThemeManager::Inst().GetThemeName(i).c_str());
			}
			SendDlgItemMessage(hDlg, IDC_THEME, CB_SETCURSEL,
				_ThemeManager::Inst().FindTheme(g_level->_infoTheme.c_str()), 0);
		} return TRUE;
	} // end of switch
    return FALSE;
}



LRESULT CALLBACK dlgGetKey(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int *pKey;

	switch (message)
	{
//	IMPLEMENT_EASYMOVE(hDlg, wParam, lParam);
//	IMPLEMENT_APPEARENCE(hDlg);
	case WM_TIMER:
	{
		if( FAILED(ReadImmediateData()) )
		{
			_ASSERT(0);
		}

		int key = 0;
		while( key < 256 )
		{
			if( g_env.envInputs.keys[key] ) break;
			key++;
		}

		if( key == 256 ) return FALSE;
		if( key == DIK_ESCAPE )
		{
			EndDialog(hDlg, IDCANCEL);
			return FALSE;
		}

		*pKey = key;
		EndDialog(hDlg, IDOK);
		return FALSE;
	} break;
	case WM_INITDIALOG:
		g_pKeyboard->SetCooperativeLevel( hDlg, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
		pKey = (int *) lParam;
		SetTimer(hDlg, 0, 10, NULL);
		return TRUE;
		break;
	case WM_DESTROY:
		KillTimer(hDlg, 0);
		g_pKeyboard->Unacquire();
		break;
	}
	return FALSE;
}


void Controls_ListViewUpdate(HWND hwndLV, PLAYER *pl, BOOL bErase)
{
	if( bErase ) ListView_DeleteAllItems(hwndLV);

	LVITEM lvi = {0};
	lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
	lvi.state     = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.iItem = ListView_GetItemCount(hwndLV);

#define INSERT_ITEM(key, name){                     \
	char s[40] = {0};                               \
	GetKeyName(pl->KeyMap.key, s);					\
	lvi.pszText = name;								\
	lvi.lParam = (LPARAM) &pl->KeyMap.key;			\
	if( bErase ) ListView_InsertItem(hwndLV, &lvi);	\
	else lvi.iItem--;								\
	ListView_SetItemText(hwndLV, lvi.iItem, 1, s);}

	INSERT_ITEM(keyTowerRight,	"Оружие направо");
	INSERT_ITEM(keyTowerCenter,	"Оружие в центр");
	INSERT_ITEM(keyTowerLeft,	"Оружие налево");
	INSERT_ITEM(keyDrop,		"Подобрать предмет");
	INSERT_ITEM(keyFire,		"Огонь!");
	INSERT_ITEM(keyLight,		"Вкл/Выкл фары");
	INSERT_ITEM(keyRight,		"Поворот направо");
	INSERT_ITEM(keyLeft,		"Поворот налево");
	INSERT_ITEM(keyBack,		"Назад");
	INSERT_ITEM(keyForward,		"Вперед");

#undef INSERT_ITEM
}

void EditKey(HWND hwndLV, PLAYER *pl)
{
	if( GetFocus() != hwndLV ) return;

	LVITEM lvi = {0};
	lvi.mask   = LVIF_PARAM;
	lvi.iItem  = ListView_GetSelectionMark(hwndLV);
	ListView_GetItem(hwndLV, &lvi);
	DialogBoxParam(g_hInstance, (LPCTSTR)IDD_DIALOG_GETKEY,
		GetParent(hwndLV), (DLGPROC) dlgGetKey, lvi.lParam);
	Controls_ListViewUpdate(hwndLV, pl, FALSE);
}

LRESULT CALLBACK dlgControls(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static PLAYER *pl;
	static HWND hwndLV;

	int wmId, wmEvent;
	switch (message)
	{
//	IMPLEMENT_EASYMOVE(hDlg, wParam, lParam);
//	IMPLEMENT_APPEARENCE(hDlg);
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case IDCANCEL:
			if( GETCHECK(IDC_RADIO_MOUSE    ) ) pl->ControlType = CT_USER_MOUSE;
			if( GETCHECK(IDC_RADIO_MOUSE2   ) ) pl->ControlType = CT_USER_MOUSE2;
			if( GETCHECK(IDC_RADIO_KEYBOARD ) ) pl->ControlType = CT_USER_KB;
			if( GETCHECK(IDC_RADIO_KEYBOARD2) ) pl->ControlType = CT_USER_KB2;
			if( GETCHECK(IDC_RADIO_HYBRID   ) ) pl->ControlType = CT_USER_HYBRID;
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
			break;
		case IDOK:
			EditKey(hwndLV, pl);
			break;
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code)
		{
//		case NM_CUSTOMDRAW:
//			SetWindowLong(hDlg, DWL_MSGRESULT, OnCustomDraw((LPNMLVCUSTOMDRAW) lParam));
//			return TRUE;
		case LVN_ITEMACTIVATE:
			EditKey(hwndLV, pl);
			break;
		}
		break;

	case WM_INITDIALOG:
		pl = (PLAYER*)lParam;

		SETCHECK(IDC_RADIO_KEYBOARD,  pl->ControlType == CT_USER_KB);
		SETCHECK(IDC_RADIO_KEYBOARD2, pl->ControlType == CT_USER_KB2);
		SETCHECK(IDC_RADIO_MOUSE,     pl->ControlType == CT_USER_MOUSE);
		SETCHECK(IDC_RADIO_MOUSE2,    pl->ControlType == CT_USER_MOUSE2);
		SETCHECK(IDC_RADIO_HYBRID,    pl->ControlType == CT_USER_HYBRID);


		hwndLV = GetDlgItem(hDlg, IDC_LIST);

		ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);

		RECT rect;
		GetClientRect(hwndLV, &rect);

		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;

		lvc.cx = 150;
		lvc.iSubItem = 0;
		lvc.pszText = "Действие";
		ListView_InsertColumn(hwndLV, 0, &lvc);

		lvc.cx = rect.right - lvc.cx;
		lvc.iSubItem = 1;
		lvc.pszText = "Кнопка";
		ListView_InsertColumn(hwndLV, 1, &lvc);

		Controls_ListViewUpdate(hwndLV, pl, TRUE);
		ShowWindow(hwndLV, SW_SHOW);
		return TRUE;
	}

	return FALSE;
}

/*
LRESULT CALLBACK dlgCreateServer(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
//	IMPLEMENT_EASYMOVE(hDlg, wParam, lParam);
//	IMPLEMENT_APPEARENCE(hDlg);
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case IDOK:
		{
			char path[MAX_PATH];
			char fn[MAX_PATH];
			SendDlgItemMessage(hDlg, IDC_MAPLIST, LB_GETTEXT,
				SendDlgItemMessage(hDlg, IDC_MAPLIST, LB_GETCURSEL, 0, 0), (LPARAM) fn);
			GetDlgItemText(hDlg, IDC_NAME, OPT(cServerName), MAX_SRVNAME);

			wsprintf(path, "%s\\%s", DIR_MAPS, fn);

			GAMEINFO gi = {0};
			gi.dwVersion  = VERSION;
			gi.dwMapCRC32 = CalcCRC32(path);
			gi.seed       = rand();
			gi.fraglimit  = __max(0, __min(MAX_FRAGLIMIT, GetDlgItemInt(hDlg, IDC_FRAGLIMIT, NULL, FALSE)));
			gi.timelimit  = __max(0, __min(MAX_TIMELIMIT, GetDlgItemInt(hDlg, IDC_TIMELIMIT, NULL, FALSE)));
			gi.server_fps = __max(MIN_NETWORKSPEED, __min(MAX_NETWORKSPEED, GetDlgItemInt(hDlg, IDC_NETWORKSPEED, NULL, FALSE)));
			gi.latency    = __max(MIN_LATENCY, __min(MAX_LATENCY, GetDlgItemInt(hDlg, IDC_LATENCY, NULL, FALSE)));
			gi.nightmode  = OPT(bNightMode) = GETCHECK(IDC_NIGHTMODE);
			strcpy(gi.cMapName, fn);
			strcpy(gi.cServerName, OPT(cServerName));

			SAFE_DELETE(g_level);
			g_level = new Level();
			g_level->_server = new TankServer();
			if( !g_level->_server->init(&gi) )
			{
				SAFE_DELETE(g_level);
				MessageBoxT(hDlg, "Не удалось запустить сервер. "
					"Проверьте конфигурацию сети", MB_OK|MB_ICONHAND);
				break;
			}

			strcpy(OPT(cMapName), fn);
			OPT(latency) = gi.latency;

			HWND hWndParent = GetParent(hDlg);
			EndDialog(hDlg, wmId);
			LOGOUT_1("DialogBox(IDD_CONNECT)\n");
			DialogBoxParam(g_hInstance, (LPCTSTR)IDD_CONNECT,
				hWndParent, (DLGPROC) dlgConnect, (LPARAM) "localhost");
		} break;
		case IDCANCEL:
			EndDialog(hDlg, wmId);
			break;
		}
		break;

	case WM_INITDIALOG:
		//
		// заполняем список карт
		//

		if( !SafeSetCurDir(DIR_MAPS, hDlg) )
		{
			EndDialog(hDlg, 0);
			return FALSE;
		}

		WIN32_FIND_DATA fd = {0};
		HANDLE hSearch = FindFirstFile("*\0", &fd);

		do {
			if (!(FILE_ATTRIBUTE_DIRECTORY & fd.dwFileAttributes))
			if (CheckFile_ie(fd.cFileName))
			{
				SendDlgItemMessage(hDlg, IDC_MAPLIST, LB_ADDSTRING, 0, (LPARAM) fd.cFileName);
			}
		} while (FindNextFile(hSearch, &fd));

		FindClose(hSearch);
		SetCurrentDirectory("..");


		//
		// находим в списке последнюю карту и выбираем ее
		//

		LONG index = SendDlgItemMessage(hDlg, IDC_MAPLIST, LB_FINDSTRING,
										(WPARAM) (-1), (LPARAM) g_options.cMapName);
		if (LB_ERR != index)
			SendDlgItemMessage(hDlg, IDC_MAPLIST, LB_SETCURSEL, index, 0);
		else
			SendDlgItemMessage(hDlg, IDC_MAPLIST, LB_SETCURSEL, 0, 0);


		//
		// установка опций
		//

		SendDlgItemMessage(hDlg, IDC_SPIN_FL, UDM_SETRANGE, 0, MAKELONG(MAX_FRAGLIMIT, 0));
		SetDlgItemInt(hDlg, IDC_FRAGLIMIT, __max(0, __min(MAX_FRAGLIMIT, g_options.fraglimit)), FALSE);
		SendDlgItemMessage(hDlg, IDC_SPIN_TL, UDM_SETRANGE, 0, MAKELONG(MAX_TIMELIMIT, 0));
		SetDlgItemInt(hDlg, IDC_TIMELIMIT, __max(0, __min(MAX_TIMELIMIT, g_options.timelimit)), FALSE);

		SendDlgItemMessage(hDlg, IDC_SPIN_NW, UDM_SETRANGE, 0, MAKELONG(MAX_NETWORKSPEED, MIN_NETWORKSPEED));
		SetDlgItemInt(hDlg, IDC_NETWORKSPEED, __max(MIN_NETWORKSPEED, __min(MAX_NETWORKSPEED, OPT(serverFps))), FALSE);
		SendDlgItemMessage(hDlg, IDC_SPIN_LATENCY, UDM_SETRANGE, 0, MAKELONG(MAX_LATENCY, MIN_LATENCY));
		SetDlgItemInt(hDlg, IDC_LATENCY, __max(MIN_LATENCY, __min(MAX_LATENCY, OPT(latency))), FALSE);



		SendDlgItemMessage(hDlg, IDC_NAME, EM_SETLIMITTEXT, MAX_SRVNAME-1, 0);
		SetDlgItemText(hDlg, IDC_NAME, OPT(cServerName));


		return TRUE;
	}
	return FALSE;
}


LRESULT CALLBACK dlgConnect(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char *autoconnect = NULL;
	int wmId, wmEvent;
	switch (message)
	{
//	IMPLEMENT_EASYMOVE(hDlg, wParam, lParam);
//	IMPLEMENT_APPEARENCE(hDlg);
	case WM_NEWDATA:
	{
		DataBlock db;
		bool ok = g_level->_client->GetData(db);
		_ASSERT(ok);

		switch( db.type() )
		{
		case DBTYPE_GAMEINFO:
		{
			GAMEINFO &gi = db.cast<GAMEINFO>();

			if( VERSION != gi.dwVersion )
			{
				SendDlgItemMessage(hDlg, IDC_STATUSWINDOW,
					LB_INSERTSTRING, -1, (LPARAM) "Несовместимая версия сервера");
                SAFE_DELETE(g_level);
				EnableWindow(GetDlgItem(hDlg, IDC_NAME), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDOK    ), TRUE);
				break;
			}


			OPT(timelimit) = gi.timelimit;
			OPT(fraglimit) = gi.fraglimit;
			OPT(serverFps) = gi.server_fps;
			OPT(bNightMode) = gi.nightmode;

			strcpy(OPT(cServerName), gi.cServerName);

			char msg[MAX_PATH + 32];
			sprintf(msg, "Загрузка карты '%s'...", gi.cMapName);
			SendDlgItemMessage(hDlg, IDC_STATUSWINDOW, LB_INSERTSTRING, -1, (LPARAM) msg);
			UpdateWindow(GetDlgItem(hDlg, IDC_STATUSWINDOW));

			char path[MAX_PATH];
			wsprintf(path, "%s\\%s", DIR_MAPS, gi.cMapName);

			if( CalcCRC32(path) != gi.dwMapCRC32 )
			{
				SendDlgItemMessage(hDlg, IDC_STATUSWINDOW,
					LB_INSERTSTRING, -1, (LPARAM) "Несовместимая версия карты");
                SAFE_DELETE(g_level);
				EnableWindow(GetDlgItem(hDlg, IDC_NAME), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDOK    ), TRUE);
				break;
			}

			if( g_level->init_newdm(path) )
				strcpy(OPT(cMapName), gi.cMapName);
			else
			{
				SendDlgItemMessage(hDlg, IDC_STATUSWINDOW,
					LB_INSERTSTRING, -1, (LPARAM) "Не удалось загрузить карту");
                SAFE_DELETE(g_level);
				EnableWindow(GetDlgItem(hDlg, IDC_NAME), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDOK    ), TRUE);
				break;
			}

			HWND hWndParent = GetParent(hDlg);
			EndDialog(hDlg, 0);
			DialogBoxParam(g_hInstance, (LPCTSTR)IDD_NETPLAYERS,
				hWndParent, (DLGPROC) dlgNetPlayers, (LPARAM) 0 );
		} break;
		case DBTYPE_ERRORMSG:
			SAFE_DELETE(g_level);
			EnableWindow(GetDlgItem(hDlg, IDC_NAME), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDOK    ), TRUE);
		case DBTYPE_TEXTMESSAGE:
			SendDlgItemMessage(hDlg, IDC_STATUSWINDOW, LB_INSERTSTRING, -1, (LPARAM) db.data() );
			break;
		default:
			_ASSERT(FALSE);
		}

		if( !g_level ) break;
	} break; // end of case WM_NEWDATA
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case IDOK:
		{
			EnableWindow(GetDlgItem(hDlg, IDC_NAME), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDOK    ), FALSE);

			char name[MAX_PATH];
			GetDlgItemText(hDlg, IDC_NAME, name, MAX_PATH);
			SendDlgItemMessage(hDlg, IDC_STATUSWINDOW, LB_RESETCONTENT, 0, 0);

			if( !autoconnect )
			{
				strcpy(OPT(cServerAddr), name);
				SAFE_DELETE(g_level);
				g_level = new Level();
			}

			_ASSERT( NULL != g_level );
			_ASSERT( NULL == g_level->_client );

			g_level->_client = new TankClient();
			g_level->_client->SetWindow(hDlg);

			if( !g_level->_client->Connect(name, g_env.hMainWnd) )
			{
				g_level->_client->ShutDown();
				SAFE_DELETE(g_level->_client);
				SendDlgItemMessage(hDlg, IDC_STATUSWINDOW, LB_INSERTSTRING, -1, (LPARAM) "Ошибка сети!");

				EnableWindow(GetDlgItem(hDlg, IDC_NAME), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDOK    ), TRUE);
			}
		} break;
		case IDCANCEL:
			SAFE_DELETE(g_level);
			EndDialog(hDlg, wmId);
			break;
		}
		break;

	case WM_INITDIALOG:
		autoconnect = (char *) lParam;
		SendDlgItemMessage(hDlg, IDC_NAME, EM_SETLIMITTEXT, MAX_PATH-1, 0);

		if( autoconnect )
		{
			SetDlgItemText(hDlg, IDC_NAME, autoconnect);
			PUSH(hDlg, IDOK);
		}
		else
		{
			SetDlgItemText(hDlg, IDC_NAME, OPT(cServerAddr));
		}

		return TRUE;
	}
    return FALSE;
}

void OutputTextBox(HWND hWndTB, LPCTSTR fmt, ...)
{
    static TCHAR tach[2048];
    va_list va;

    va_start(va, fmt);
    vsprintf (tach, fmt, va);
    va_end(va);

	//-------------------------------

	string_t str = tach;

	int l = str.size();
	bool just_r = false;
	for( int i = 0; i < l; ++i )
	{
		if( !just_r && '\n' == str[i] )
		{
			str = str.substr(0, i) + "\r" + str.substr(i, l);
			++l;
		}

		just_r = ('\r' == str[i]);
	}

	//-------------------------------

	SendMessage(hWndTB, EM_SETSEL, 0x7fffffff, 0x7fffffff);
	SendMessage(hWndTB, EM_REPLACESEL, (WPARAM) FALSE, (LPARAM) str.c_str());
}

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

				for( size_t i = 0; i < g_level->_client->_dwLatency; ++i )
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
