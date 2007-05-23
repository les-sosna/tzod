// interface.cpp
//

#include "stdafx.h"

#include "interface.h"
#include "SaveLoad.h"
#include "macros.h"
#include "options.h"
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
#include "gc/Editor.h"
#include "gc/GameClasses.h"
#include "gc/RigidBody.h"

#include "res/resource.h"

////////////////////////////////////////////////////////////////////

#define GETCHECK(id)  (BST_CHECKED == SendDlgItemMessage(hDlg, (id), BM_GETCHECK, 0, 0))
#define SETCHECK(id, value) SendDlgItemMessage(hDlg, (id), BM_SETCHECK, (value) ? BST_CHECKED:BST_UNCHECKED, 0)

/////////////////////////////////////////////////////////////////////

void MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASS wc = {0};

	//
	// main window
	//
    wc.lpszClassName = TXT_WNDCLASS;
    wc.lpfnWndProc   = (WNDPROC) WndProc;
	wc.style         = CS_VREDRAW | CS_HREDRAW;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, (LPCTSTR) IDI_BIG);
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = NULL;
	RegisterClass(&wc);

	//
	// my property grid
	//
    wc.lpszClassName = TXT_PROPGRID;
    wc.lpfnWndProc   = (WNDPROC) PropGridProc;
	wc.style         = CS_VREDRAW | CS_HREDRAW;
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = NULL;//g_Interface.hbrBackground;
	RegisterClass(&wc);

	//
	// my property grid internal
	//
    wc.lpszClassName = TXT_PROPGRIDINT;
    wc.lpfnWndProc   = (WNDPROC) PropGridIntProc;
	wc.style         = CS_VREDRAW | CS_HREDRAW;
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = NULL;//g_Interface.hbrBackground;
	RegisterClass(&wc);
}

///////////////////////////////////////////////////////////////////////////////////////////

/*
void GetVehClassesList(std::vector<std::pair<string_t, string_t> > &ls)
{
	std::pair<string_t, string_t> val;
	lua_State *L = g_env.hScript;

	lua_getglobal(L, "classes");

	// loop over files
	for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
        val.first = lua_tostring(L, -2);
		val.second = lua_tostring(L, -2); //lua_tostring(L, -1);

		ls.push_back(val);
	}
}
*/

///////////////////////////////////////////////////////////////////////////////////////////

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
			if( g_level && !g_level->_client )
				g_level->_timer.Stop();
		}
		else if( g_env.minimized )
		{
			TRACE("main window activated\n");

			g_env.minimized = false;
			if( g_level && !g_level->_client )
				g_level->_timer.Start();
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
				g_gui->Resize((float) g_render->getXsize(), (float) g_render->getYsize());
		}
		break;

	case WM_SETCURSOR:
		if( g_level && g_level->_timer.IsRuning() && (HTCLIENT & lParam) )
			SetCursor(NULL);
		else
			DefWindowProc(hWnd, message, wParam, lParam);
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
		OnMouse(message, wParam, lParam);
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
//	case WM_PAINT:
//		if( g_render ) RenderFrame(false);
//		ValidateRect(hWnd, NULL);
//		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK PropGridProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static IPropertySet* p = NULL;
	SCROLLINFO si = { sizeof(SCROLLINFO) };

	switch( message )
	{
	case WM_UPDATE_DATA:
	{
		HWND hWndScroll = GetTopWindow(hWnd);

		for( int i = 0; i < p->GetCount(); ++i )
		{
			ObjectProperty *prop = p->GetProperty(i);
			HWND hWndControl = GetDlgItem(hWndScroll, i+1024);
			_ASSERT(NULL != hWndControl);

			switch( prop->GetType() )
			{
			case ObjectProperty::TYPE_INTEGER:
				prop->SetValueInt(SendMessage(hWndControl, UDM_GETPOS32, 0, 0));
				break;
			case ObjectProperty::TYPE_STRING:
				char *buf;
				int len;
				len = GetWindowTextLength(hWndControl);
				buf = new char[len+1];
				GetWindowText(hWndControl, buf, len);
				prop->SetValue(buf);
				delete[] buf;
				break;
			case ObjectProperty::TYPE_MULTISTRING:
				prop->SetCurrentIndex(SendMessage(hWndControl, CB_GETCURSEL, 0, 0));
				break;
			default:
				_ASSERT(FALSE);
			} // end of switch( prop->GetType() )
		}
		p->Exchange(true);
	} break;
	case WM_CREATE:
	{
		CREATESTRUCT *cs = (CREATESTRUCT *) lParam;
		if( cs->lpCreateParams )
		{
			HFONT hFont = (HFONT) SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0);
			p = (IPropertySet *) cs->lpCreateParams;

			RECT rt;
			GetClientRect(hWnd, &rt);
			rt.right -= 5;

			si.fMask  = SIF_PAGE | SIF_RANGE /* | SIF_DISABLENOSCROLL*/;
			si.nMin   = 0;
			si.nMax   = PROPERTY_GRID_ROW_HEIGHT * p->GetCount() + PROPERTY_GRID_ROW_VSPACE*(p->GetCount()+2);
			si.nPage  = HEIGHT(rt);
			SetScrollInfo(hWnd, SB_VERT, &si, FALSE);

			HWND hWndScroll = CreateWindowEx(
				WS_EX_CONTROLPARENT, TXT_PROPGRIDINT, "", WS_CHILD|WS_VISIBLE,
				0, 0, WIDTH(rt), si.nMax, hWnd, NULL, g_hInstance, NULL
			);

			for( int i = 0; i < p->GetCount(); ++i )
			{
				ObjectProperty *prop = p->GetProperty(i);
				int y = PROPERTY_GRID_ROW_HEIGHT*i + PROPERTY_GRID_ROW_VSPACE*(i+1);

				HWND hWndLabel = CreateWindowEx(
					WS_EX_TRANSPARENT,                      // ex style
					"STATIC",                               // class
					prop->GetName().c_str(),                // name
					WS_CHILD|WS_VISIBLE | SS_RIGHT|SS_CENTERIMAGE,     // style
					0,                                      // x
					y,                                      // y
					PROPERTY_GRID_LABEL_WIDTH-5,            // width
					PROPERTY_GRID_ROW_HEIGHT,               // height
					hWndScroll, NULL, g_hInstance, NULL
				);
				SendMessage(hWndLabel, WM_SETFONT, (WPARAM) hFont, 1);

				HWND hWndControl, tmp;
				switch( prop->GetType() )
				{
				case ObjectProperty::TYPE_INTEGER:
					tmp = CreateWindowEx(
						WS_EX_CLIENTEDGE,               // ex style
						"EDIT",                                           // class
						"",	                                              // name
						WS_CHILD|WS_VISIBLE|WS_TABSTOP
							| ES_AUTOHSCROLL|ES_LEFT|ES_NUMBER,	    // style
						PROPERTY_GRID_LABEL_WIDTH,						  // x
						y,                                                // y
						WIDTH(rt)-PROPERTY_GRID_LABEL_WIDTH,			  // width
						PROPERTY_GRID_ROW_HEIGHT,						  // height
						hWndScroll, NULL, g_hInstance, NULL
					);
					SendMessage(tmp, WM_SETFONT, (WPARAM) hFont, 0);
					hWndControl = CreateWindow(
						UPDOWN_CLASS,
						"",
						WS_CHILD|WS_VISIBLE | UDS_SETBUDDYINT|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS/*|UDS_ARROWKEYS*/,
						0,0,0,0,
						hWndScroll,
						NULL,
						g_hInstance,
						NULL
					);
					SendMessage(hWndControl, UDM_SETBUDDY, (WPARAM) tmp, 0);
					SendMessage(hWndControl, UDM_SETRANGE32, prop->GetMin(), prop->GetMax());
					SendMessage(hWndControl, UDM_SETPOS32, 0, prop->GetValueInt());
					break;
				case ObjectProperty::TYPE_STRING:
					hWndControl = CreateWindowEx(
						WS_EX_TRANSPARENT|WS_EX_CLIENTEDGE,               // ex style
						"EDIT",                                           // class
						prop->GetValue().c_str(),				          // name
						WS_CHILD|WS_VISIBLE|WS_TABSTOP
							| ES_AUTOHSCROLL|ES_LEFT,	            // style
						PROPERTY_GRID_LABEL_WIDTH,						  // x
						y,                                                // y
						WIDTH(rt)-PROPERTY_GRID_LABEL_WIDTH,			  // width
						PROPERTY_GRID_ROW_HEIGHT,						  // height
						hWndScroll, NULL, g_hInstance, NULL
					);
					SendMessage(hWndControl, WM_SETFONT, (WPARAM) hFont, 0);
					break;
				case ObjectProperty::TYPE_MULTISTRING:
					hWndControl = CreateWindowEx(
						WS_EX_NOPARENTNOTIFY,                             // ex style
						"ComboBox",                                       // class
						prop->GetName().c_str(),				          // name
						WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|
						    CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS, // style
						PROPERTY_GRID_LABEL_WIDTH,						  // x
						y,                                                // y
						WIDTH(rt)-PROPERTY_GRID_LABEL_WIDTH,			  // width
						PROPERTY_GRID_ROW_HEIGHT*5,						  // height
						hWndScroll, NULL, g_hInstance, NULL
					);
					SendMessage(hWndControl, WM_SETFONT, (WPARAM) hFont, 0);
					SendMessage(hWndControl, CB_SETITEMHEIGHT, -1,
						PROPERTY_GRID_ROW_HEIGHT - GetSystemMetrics(SM_CXFIXEDFRAME)*2);
					SendMessage(hWndControl, CB_SETITEMHEIGHT,  0,
						PROPERTY_GRID_ROW_HEIGHT - GetSystemMetrics(SM_CXFIXEDFRAME)*2);
					for( size_t index = 0; index < prop->GetSetSize(); ++index )
					{
						SendMessage(hWndControl, CB_ADDSTRING,
							0, (LPARAM) prop->GetSetValue(index).c_str());
					}
					SendMessage(hWndControl, CB_SETCURSEL, (WPARAM) prop->GetCurrentIndex(), 0);
					break;
				default:
					_ASSERT(FALSE);
				} // end of switch( prop->GetType() )
				SetWindowLong(hWndControl, GWL_ID, i+1024);
			}
		}
	} break;
	case WM_MOUSEWHEEL:
	{
		int d = (short) HIWORD(wParam);
		WPARAM cmd = d < 0 ? SB_LINEDOWN : SB_LINEUP;
		d = abs(d) / 120;  // WHEEL_DELTA
		for( int i = 0; i < d; ++i )
			SendMessage(hWnd, WM_VSCROLL, cmd, 0);
	} break;
	case WM_VSCROLL:
	{
		_ASSERT(NULL != p);
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
		GetScrollInfo(hWnd, SB_VERT, &si);
		int pos = si.nPos;
		switch(LOWORD(wParam))
		{
		case SB_LINEUP:
			pos -= PROPERTY_GRID_ROW_HEIGHT;
			break;
		case SB_LINEDOWN:
			pos += PROPERTY_GRID_ROW_HEIGHT;
			break;
		case SB_PAGEUP:
			pos -= si.nPage;
			break;
		case SB_PAGEDOWN:
			pos += si.nPage;
			break;
		case SB_TOP:
			pos = si.nMin;
			break;
		case SB_BOTTOM:
			pos = si.nMax;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			pos = si.nTrackPos;
			break;
		}
		if( pos > si.nMax - (int) si.nPage ) pos = si.nMax - (int) si.nPage;
		if( pos < si.nMin ) pos = si.nMin;
		SetScrollPos(hWnd, SB_VERT, pos, TRUE);
		SetWindowPos(GetTopWindow(hWnd), NULL, 0, -pos, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	} break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	} // end of switch( message )

	return 0;
}


LRESULT CALLBACK PropGridIntProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, message, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////////////////

BOOL Analisis_btn(LPDRAWITEMSTRUCT pdi_old, LPDRAWITEMSTRUCT pdi_new)
{
	if (GetParent(pdi_old->hwndItem) !=
		GetParent(pdi_new->hwndItem)) return FALSE;

	if (0 == (pdi_new->itemAction & ODA_FOCUS)) return FALSE;
	if (0 == (pdi_new->itemState  & ODS_FOCUS)) return FALSE;

	if (0x8000 & GetKeyState(VK_LBUTTON)) return FALSE;

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////

BOOL Analisis_cb(LPDRAWITEMSTRUCT pdi_old, LPDRAWITEMSTRUCT pdi_new)
{
	if (GetParent(pdi_old->hwndItem) !=
		GetParent(pdi_new->hwndItem)) return FALSE;

	if (0 == (pdi_new->itemAction & ODA_FOCUS)) return FALSE;
	if (0 == (pdi_new->itemState  & ODS_FOCUS)) return FALSE;

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////

void AddPlayerToListView(HWND hwndLV, char *pName, char *pSkinName, char *pClassName,
						 int type, int team, LPVOID param = 0, int index = -1)
{
	LVITEM lvi = {0};
	lvi.mask      = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
	lvi.pszText   = pName;
	lvi.lParam    = (LPARAM) param;
	lvi.state     = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

	int item;
	if (-1 == index)
	{
		lvi.iItem = ListView_GetItemCount(hwndLV);
		item = ListView_InsertItem(hwndLV, &lvi);
	}
	else
	{
		lvi.iItem = item = index;
		ListView_SetItem(hwndLV, &lvi);
	}

	char skin[MAX_PATH];
	strcpy(skin, pSkinName);
	ListView_SetItemText(hwndLV, item, 1, skin);


	//if( OPT(players[type]).bAI )
	//{
	//	ListView_SetItemText(hwndLV, item, 2, "Комп.")
	//}
	//else
	//{
	//	char s[16];
	//	wsprintf(s, "Человек %d", type + 1);
	//	ListView_SetItemText(hwndLV, item, 2, s);
	//}

	ListView_SetItemText(hwndLV, item, 2, pClassName)




	char s[16];
	if (0 != team) wsprintf(s, "%d", team);
	else wsprintf(s, "[нет]");
	ListView_SetItemText(hwndLV, item, 3, s);
	ListView_SetSelectionMark(hwndLV, item);
}

void AddPlayerToListView(HWND hwndLV, char *pName, char *pSkinName, int team, LPVOID param = 0, int index = -1)
{
	LVITEM lvi = {0};
	lvi.mask      = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
	lvi.pszText   = pName;
	lvi.lParam    = (LPARAM) param;
	lvi.state     = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

	int item;
	if (-1 == index)
	{
		lvi.iItem = ListView_GetItemCount(hwndLV);
		item = ListView_InsertItem(hwndLV, &lvi);
	}
	else
	{
		lvi.iItem = item = index;
		ListView_SetItem(hwndLV, &lvi);
	}

	char skin[MAX_PATH];
	strcpy(skin, pSkinName);
	ListView_SetItemText(hwndLV, item, 1, skin);

	char s[16];
	if (0 != team) wsprintf(s, "%d", team);
	else wsprintf(s, "[нет]");
	ListView_SetItemText(hwndLV, item, 2, s);

	ListView_SetSelectionMark(hwndLV, item);
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

	if (MB_OKCANCEL & uType)
		templ = (LPCTSTR) IDD_MSGBOX_OKCANCEL;
	else if (MB_YESNO & uType)
		templ = (LPCTSTR) IDD_MSGBOX_YESNO;
	else
		templ = (LPCTSTR) IDD_MSGBOX_OK;

	return DialogBoxParam(g_hInstance, templ, hWnd, (DLGPROC) dlgMsgBox, (LPARAM) lpText);
}

///////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK dlgObjectProperties(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hWndPropGrid;
	RECT rt;

	int wmId, wmEvent;
	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
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
		case BN_CLICKED:
			switch (wmId)
			{
			case IDOK:
				SendMessage(hWndPropGrid, WM_UPDATE_DATA, 0, 0);
			case IDCANCEL:
				EndDialog(hDlg, wmId);
				break;
			}
			break;
		}
		break;
	case WM_INITDIALOG:
		GetClientRect(hDlg, &rt);
		hWndPropGrid = CreateWindowEx(WS_EX_CLIENTEDGE|WS_EX_CONTROLPARENT,
			TXT_PROPGRID,
			"",
			WS_CHILD|WS_VSCROLL|WS_VISIBLE|WS_TABSTOP,
			10, 10,
			WIDTH(rt) - 20,
			PROPERTY_GRID_ROW_HEIGHT*8 + GetSystemMetrics(SM_CXFIXEDFRAME)*2,
			hDlg, NULL, g_hInstance, (LPVOID) lParam
		);
		return TRUE;
	}
    return FALSE;
}

LRESULT CALLBACK dlgNewMap(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y;
	int wmId, wmEvent;
	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
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
				OPT(xsize) = __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, GetDlgItemInt(hDlg, IDC_X, NULL, FALSE)));
				OPT(ysize) = __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, GetDlgItemInt(hDlg, IDC_Y, NULL, FALSE)));
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
		SetDlgItemInt(hDlg, IDC_X, __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, g_options.xsize)), FALSE);
		SetDlgItemInt(hDlg, IDC_Y, __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, g_options.ysize)), FALSE);
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
//	IMPLEMENT_EASYMOVE(hDlg, wParam, lParam);
//	IMPLEMENT_APPEARENCE_NP(hDlg);
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
		{
//		case BN_KILLFOCUS:
//			SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
//			break;
//		case BN_SETFOCUS:
//			SendMessage(hDlg, DM_SETDEFID, wmId, 0);
//			break;
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
LRESULT CALLBACK dlgAddPlayer(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	static LPPLAYERDESCEX ppdex = NULL;
	static int frame = 0;
	static std::vector<string_t> skins;
	static std::vector<std::pair<string_t, string_t> > classes;


	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case IDOK:
			// комманда
			ppdex->team = SendDlgItemMessage(hDlg, IDC_TEAM, CB_GETCURSEL, 0, 0);

			// скин
			int skin;
			skin = SendDlgItemMessage(hDlg, IDC_SKIN, CB_GETCURSEL, 0, 0);
			if( -1 == skin ) skin = 0;
			strcpy(ppdex->skin, skins[skin].c_str());

			// контроллер (тип)
			ppdex->type = (short) SendDlgItemMessage(hDlg, IDC_TYPE, CB_GETITEMDATA,
				SendDlgItemMessage(hDlg, IDC_TYPE, CB_GETCURSEL, 0, 0), 0);

			// класс
			strcpy(ppdex->cls, ((string_t *) SendDlgItemMessage(hDlg, IDC_CLASS, CB_GETITEMDATA,
				SendDlgItemMessage(hDlg, IDC_CLASS, CB_GETCURSEL, 0, 0), 0))->c_str());

			// имя
			GetDlgItemText(hDlg, IDC_NAME, ppdex->name, MAX_PLRNAME);

			_ASSERT(-1 != ppdex->type);
			_ASSERT(-1 != ppdex->team);
			EndDialog(hDlg, wmId);
			break;
		case IDCANCEL:
			EndDialog(hDlg, wmId);
			break;
		}
		break;

	case WM_TIMER:
		RECT rect;
		if ((++frame) == 64) frame = 0;
		GetWindowRect(GetDlgItem(hDlg, IDC_PREVIEW), &rect);
		ScreenToClient(hDlg, (LPPOINT) &rect);
		ScreenToClient(hDlg, (LPPOINT) &rect + 1);
		InvalidateRect(hDlg, &rect, FALSE);
		break;

	case WM_INITDIALOG:
		ppdex = (LPPLAYERDESCEX) lParam;

		// name
		SendDlgItemMessage(hDlg, IDC_NAME, EM_SETLIMITTEXT, MAX_PLRNAME, 0);
		SetDlgItemText(hDlg, IDC_NAME, ppdex->name);

		SendDlgItemMessage(hDlg, IDC_SKIN,  CB_SETEXTENDEDUI, TRUE, 0);
		SendDlgItemMessage(hDlg, IDC_TYPE,  CB_SETEXTENDEDUI, TRUE, 0);
		SendDlgItemMessage(hDlg, IDC_CLASS, CB_SETEXTENDEDUI, TRUE, 0);
		SendDlgItemMessage(hDlg, IDC_TEAM,  CB_SETEXTENDEDUI, TRUE, 0);

		// skins
		g_texman->GetTextureNames(skins, "skin/");
		for( size_t i = 0; i < skins.size(); ++i )
			SendDlgItemMessage(hDlg, IDC_SKIN, CB_ADDSTRING, 0, (LPARAM) skins[i].c_str() );
		size_t j = std::find(skins.begin(), skins.end(), ppdex->skin) - skins.begin();
		SendDlgItemMessage(hDlg, IDC_SKIN, CB_SETCURSEL, (skins.size() != j) ? j : 0, 0);

		// controller types
		int index = 0;
		for( int type = 0; type < MAX_HUMANS; ++type )
		{
			if( !(ppdex->dwHasPlayers & (1 << type)) )
			{
				char s[16];
				wsprintf(s, "человек %d", type+1);
				SendDlgItemMessage(hDlg, IDC_TYPE, CB_ADDSTRING, 0, (LPARAM) s);
				SendDlgItemMessage(hDlg, IDC_TYPE, CB_SETITEMDATA, index, (LPARAM) type);
				if (ppdex->type == type)
					SendDlgItemMessage(hDlg, IDC_TYPE, CB_SETCURSEL, index, 0);
				++index;
			}
		}
		SendDlgItemMessage(hDlg, IDC_TYPE, CB_ADDSTRING, index, (LPARAM) "компьютер");
		SendDlgItemMessage(hDlg, IDC_TYPE, CB_SETITEMDATA, index, (LPARAM) MAX_HUMANS);
		if (-1 == ppdex->type)
			SendDlgItemMessage(hDlg, IDC_TYPE, CB_SETCURSEL, 0, 0);
		else if (MAX_HUMANS == ppdex->type)
			SendDlgItemMessage(hDlg, IDC_TYPE, CB_SETCURSEL, index, 0);

		// teams
		SendDlgItemMessage(hDlg, IDC_TEAM, CB_ADDSTRING, 0, (LPARAM) "[нет]");
		for( size_t i = 1; i < MAX_TEAMS; ++i )
		{
			char s[4];
			wsprintf(s, "%d", i);
			SendDlgItemMessage(hDlg, IDC_TEAM, CB_ADDSTRING, 0, (LPARAM) s);
		}
		SendDlgItemMessage(hDlg, IDC_TEAM, CB_SETCURSEL, ppdex->team, 0);

		// vehicle classes
		if( classes.empty() )
			GetVehClassesList(classes);
		index = 0;
		for( size_t i = 0; i < classes.size(); ++i )
		{
			SendDlgItemMessage(hDlg, IDC_CLASS, CB_ADDSTRING, 0,
				(LPARAM) classes[i].second.c_str());
			SendDlgItemMessage(hDlg, IDC_CLASS, CB_SETITEMDATA, i, (LPARAM) &classes[i].first);
			if( classes[i].first == ppdex->cls )
			{
				SendDlgItemMessage(hDlg, IDC_CLASS, CB_SETCURSEL, index, 0);
				index = i;
			}
		}
		SendDlgItemMessage(hDlg, IDC_CLASS, CB_SETCURSEL, index, 0);

		// skin preview timer
		SetTimer(hDlg, 0, 40, NULL);

		return TRUE;
	}
    return FALSE;
}
*/

/*
void dmUpdateUI (HWND hDlg)
{
	HWND hwndLV;
	hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);

	ListView_DeleteAllItems(hwndLV);

	if (g_options.dm_nPlayers)
	{
		EnableWindow(hwndLV, TRUE);

		for (int i = 0; i < g_options.dm_nPlayers; ++i)
		{
			AddPlayerToListView(hwndLV,
				g_options.dm_pdPlayers[i].name,
				g_options.dm_pdPlayers[i].skin,
				g_options.dm_pdPlayers[i].cls,
				g_options.dm_pdPlayers[i].type,
				g_options.dm_pdPlayers[i].team,
				(LPVOID) i);
		}
	}
	else
	{
		LVITEM lvi = {0};
		lvi.mask = LVIF_TEXT | LVIF_STATE;
		lvi.state = 0;
		lvi.stateMask = 0;

		lvi.pszText = "<нет ни одного игрока>";
		ListView_InsertItem(hwndLV, &lvi);

		EnableWindow(hwndLV, FALSE);
	}

	EnableWindow(GetDlgItem(hDlg, IDC_ADD ), g_options.dm_nPlayers < MAX_PLAYERS);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT), ListView_GetSelectedCount(hwndLV));
	EnableWindow(GetDlgItem(hDlg, IDC_KICK), ListView_GetSelectedCount(hwndLV));
	EnableWindow(GetDlgItem(hDlg, IDOK    ), (g_options.dm_nPlayers != 0 ||
		GETCHECK(IDC_ALLOW_EDITPLAYERS)) &&
		LB_ERR != SendDlgItemMessage(hDlg, IDC_MAPLIST, LB_GETCURSEL, 0, 0));
}

LRESULT CALLBACK dlgNewDM(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	PLAYERDESCEX pdex;
	static HFONT hFont;
	int x;

	int wmId, wmEvent;
	switch (message)
	{
//	IMPLEMENT_EASYMOVE(hDlg, wParam, lParam);
//	IMPLEMENT_APPEARENCE(hDlg);
	case WM_NOTIFY:
	{
		wmId = (int) wParam;
		NMHDR *phdr = (LPNMHDR) lParam;
		switch (phdr->idFrom)
		{
		case IDC_MAPLIST:
			switch (phdr->code)
			{
			case LVN_KEYDOWN:
				switch (((LPNMLVKEYDOWN) lParam)->wVKey)
				{
				case VK_INSERT:
				case VK_DELETE:
					break;
				}
				break;
			case LVN_ITEMCHANGED:
			{
				LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
				if( LVIS_SELECTED & pnmv->uNewState )
				{
					HWND hwndLV = GetDlgItem(hDlg, IDC_MAPLIST);
					int index = pnmv->iItem;
					if( -1 != index )
					{
						char path[MAX_PATH];
						char fn[MAX_PATH];
						ListView_GetItemText(hwndLV, index, 0, fn, MAX_PATH);
						wsprintf(path, "%s\\%s.map", DIR_MAPS, fn);

						MapFile file;
						if( file.Open(path, false) )
						{
							std::stringstream str_desc;
							string_t tmp;

							if( file.getMapAttribute("desc", tmp) && !tmp.empty() )
							{
								str_desc << tmp;
								str_desc << "\r\n---------------------------\r\n";
							}

							if( file.getMapAttribute("author", tmp) && !tmp.empty() )
								str_desc << "Автор: " << tmp;
							else
								str_desc << "Автор неизвестен";
							str_desc << "\r\n";

							if( file.getMapAttribute("link-url", tmp) && !tmp.empty() )
							{
								if( 0 != tmp.find("http://") )
									str_desc << "http://";
								str_desc << tmp << "\r\n";
							}
							if( file.getMapAttribute("e-mail", tmp) && !tmp.empty() )
							{
								if( 0 != tmp.find("mailto:") )
									str_desc << "mailto:";
								str_desc << tmp << "\r\n";
							}

							HWND hwndDesc = GetDlgItem(hDlg, IDC_MAPINFO);
							SetWindowText(hwndDesc, str_desc.str().c_str());
						//	InvalidateRect(hwndDesc, NULL, TRUE);
						}
					}
				}
			} break;
			case LVN_ITEMACTIVATE:
				PUSH(hDlg, IDOK);
				break;
			case NM_SETFOCUS :
				SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
				break;
//			case NM_CUSTOMDRAW:
//				SetWindowLong(hDlg, DWL_MSGRESULT, OnCustomDraw((LPNMLVCUSTOMDRAW) lParam));
//				return TRUE;
			}
			break;
		case IDC_PLRLIST:
			switch (phdr->code)
			{
			case LVN_KEYDOWN:
				switch (((LPNMLVKEYDOWN) lParam)->wVKey)
				{
				case VK_INSERT:
					if (g_options.dm_nPlayers < MAX_PLAYERS)
						PUSH(hDlg, IDC_ADD);
					break;
				case VK_DELETE:
					if (ListView_GetSelectedCount(phdr->hwndFrom))
						PUSH(hDlg, IDC_KICK);
					break;
				}
				break;
			case LVN_ITEMCHANGED:
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT), ListView_GetSelectedCount(phdr->hwndFrom));
				EnableWindow(GetDlgItem(hDlg, IDC_KICK), ListView_GetSelectedCount(phdr->hwndFrom));
				break;
			case LVN_ITEMACTIVATE:
				PUSH(hDlg, IDC_EDIT);
				break;
			case NM_SETFOCUS :
				SendMessage(hDlg, DM_SETDEFID, IDC_EDIT, 0);
				break;
//			case NM_CUSTOMDRAW:
//				SetWindowLong(hDlg, DWL_MSGRESULT, OnCustomDraw((LPNMLVCUSTOMDRAW) lParam));
//				return TRUE;
			}
			break;
		}
	} break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
		{
		case LBN_SETFOCUS:
			SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
			break;
		case LBN_DBLCLK:
			PUSH(hDlg, IDOK);
			break;
		case BN_SETFOCUS:
			SendMessage(hDlg, DM_SETDEFID, wmId, 0);
			break;
		case BN_CLICKED:
			switch (wmId)
			{
			case IDC_ADD:
			{
				_ASSERT(g_options.dm_nPlayers < MAX_PLAYERS);
				ZeroMemory(&pdex, sizeof(PLAYERDESCEX));

				std::vector<string_t> names;
				g_texman->GetTextureNames(names, "skin/");
				strcpy(pdex.skin, names[rand() % names.size()].c_str());
				strcpy(pdex.name, "new player");


				for (x = 0; x < g_options.dm_nPlayers; ++x)
				{
					_ASSERT(-1 < g_options.dm_pdPlayers[x].type);
					pdex.dwHasPlayers |= (1 << g_options.dm_pdPlayers[x].type);
				}

				pdex.type = -1;

				LOGOUT_1("DialogBox(IDD_ADDPLAYER)\n");
				if (IDOK == DialogBoxParam(g_hInstance, (LPCTSTR)IDD_ADDPLAYER, hDlg,
					(DLGPROC) dlgAddPlayer, (LPARAM) &pdex) )
				{
					memcpy(&g_options.dm_pdPlayers[g_options.dm_nPlayers], &pdex, sizeof(PLAYERDESC));
					g_options.dm_nPlayers++;
				}

				dmUpdateUI(hDlg);
			} break;
			case IDC_EDIT:
			{
				_ASSERT(g_options.dm_nPlayers < MAX_PLAYERS);
				ZeroMemory(&pdex, sizeof(PLAYERDESCEX));

				HWND hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);
				int index = ListView_GetSelectionMark(hwndLV);
				if (-1 != index)
				{
					memcpy(&pdex, &g_options.dm_pdPlayers[index], sizeof(PLAYERDESC));

					for (x = 0; x < g_options.dm_nPlayers; ++x)
					{
						_ASSERT(-1 < g_options.dm_pdPlayers[x].type);
						if (x == index) continue;
						pdex.dwHasPlayers |= (1 << g_options.dm_pdPlayers[x].type);
					}

					LOGOUT_1("DialogBox(IDD_ADDPLAYER)\n");
					if (IDOK == DialogBoxParam(g_hInstance, (LPCTSTR)IDD_ADDPLAYER,
						hDlg, (DLGPROC) dlgAddPlayer, (LPARAM) &pdex))
					{
						memcpy(&g_options.dm_pdPlayers[index], &pdex, sizeof(PLAYERDESC));

						dmUpdateUI(hDlg);
						ListView_SetSelectionMark(hwndLV, index);
					}
				}
			} break;
			case IDC_KICK:
			{
				HWND hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);
				LVITEM lvi;
				lvi.iItem = ListView_GetSelectionMark(hwndLV);
				lvi.iSubItem = 0;
				lvi.mask = LVIF_PARAM;
				_ASSERT(-1 != lvi.iItem);

				ListView_GetItem(hwndLV, &lvi);

				if (IDYES == MessageBoxT(hDlg, "Удалить игрока?", MB_YESNO|MB_ICONQUESTION))
				{
					memmove(&g_options.dm_pdPlayers[lvi.lParam    ],
							&g_options.dm_pdPlayers[lvi.lParam + 1],
							sizeof(PLAYERDESC) * (MAX_PLAYERS - g_options.dm_nPlayers) );
					g_options.dm_nPlayers--;

					dmUpdateUI(hDlg);
				}
			} break;
			case IDC_FRAGLIMIT:
				switch (wmEvent)
				{
				case EN_KILLFOCUS:
					x = __max(0, __min(MAX_FRAGLIMIT, GetDlgItemInt(hDlg, IDC_FRAGLIMIT, NULL, FALSE)));
					SetDlgItemInt(hDlg, IDC_FRAGLIMIT, x, FALSE);
					break;
				}
				break;
			case IDC_TIMELIMIT:
				switch (wmEvent)
				{
				case EN_KILLFOCUS:
					x = __max(0, __min(MAX_TIMELIMIT, GetDlgItemInt(hDlg, IDC_TIMELIMIT, NULL, FALSE)));
					SetDlgItemInt(hDlg, IDC_TIMELIMIT, x, FALSE);
					break;
				}
				break;
			case IDOK:
			{
				char path[MAX_PATH];
				char fn[MAX_PATH];
				HWND hwndLV = GetDlgItem(hDlg, IDC_MAPLIST);
				int index = ListView_GetSelectionMark(hwndLV);
				if( -1 != index )
				{
					ListView_GetItemText(hwndLV, index, 0, fn, MAX_PATH);
				}
				else
				{
					MessageBoxT(hDlg, "Выберите карту", MB_OK|MB_ICONHAND);
					break;
				}

				wsprintf(path, "%s\\%s.map", DIR_MAPS, fn);

				OPT(gameSpeed)    = __max(MIN_GAMESPEED, __min(MAX_GAMESPEED, GetDlgItemInt(hDlg, IDC_SPEED,     NULL, FALSE)));
				OPT(fraglimit)    = __max(0, __min(MAX_FRAGLIMIT, GetDlgItemInt(hDlg, IDC_FRAGLIMIT, NULL, FALSE)));
				OPT(timelimit)    = __max(0, __min(MAX_TIMELIMIT, GetDlgItemInt(hDlg, IDC_TIMELIMIT, NULL, FALSE)));
				OPT(bNightMode)   = GETCHECK(IDC_NIGHTMODE);
				OPT(bAllowEditor) = GETCHECK(IDC_ALLOW_EDITOR);
				OPT(bAllowPlayerChange) = GETCHECK(IDC_ALLOW_EDITPLAYERS);


				if( LEVEL_INIT_PARAM(init_newdm, path) )
				{
					g_level->_seed = rand();
					strcpy(OPT(cMapName), fn);

					// добавляем игроков в обратном порядке
					for( x = g_options.dm_nPlayers - 1; x >= 0; --x )
					{
						GC_Player *pPlayer = new GC_Player(g_options.dm_pdPlayers[x].team);
						pPlayer->SetController(g_options.dm_pdPlayers[x].type);
						strcpy(pPlayer->_name, g_options.dm_pdPlayers[x].name);
						strcpy(pPlayer->_skin, g_options.dm_pdPlayers[x].skin);
						strcpy(pPlayer->_class, g_options.dm_pdPlayers[x].cls);
					}

					g_gui->Show(false);
				}
				else
				{
					MessageBoxT(hDlg, "Ошибка при загрузке карты", MB_OK|MB_ICONHAND);
					break;
				}
				EndDialog(hDlg, wmId);
			} break;
			case IDCANCEL:
				EndDialog(hDlg, wmId);
				break;
			case IDC_ALLOW_EDITPLAYERS:
				dmUpdateUI(hDlg);
			case IDC_ALLOW_EDITOR:
				break;
			}
			break;
		}
		break;

	case WM_INITDIALOG:
	{
		//
		// заполняем список карт
		//

		HWND hwndLV;
		hwndLV = GetDlgItem(hDlg, IDC_MAPLIST);

		ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);
		ListView_SetBkColor(hwndLV, ICOLOR_BACKGROUND);

		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;

		lvc.cx = 180;
		lvc.iSubItem = 0;
		lvc.pszText = "Имя карты";
		ListView_InsertColumn(hwndLV, 0, &lvc);

		lvc.cx = 70;
		lvc.iSubItem = 1;
		lvc.pszText = "Размер";
		ListView_InsertColumn(hwndLV, 1, &lvc);

		lvc.cx = 70;
		lvc.iSubItem = 2;
		lvc.pszText = "Тема";
		ListView_InsertColumn(hwndLV, 2, &lvc);

		if( !SafeSetCurDir(DIR_MAPS, hDlg) )
		{
			EndDialog(hDlg, 0);
			return FALSE;
		}

		WIN32_FIND_DATA fd = {0};
		HANDLE hSearch = FindFirstFile("*.map\0", &fd);

		int last_map_index = 0;

		do
		{
			if( !(FILE_ATTRIBUTE_DIRECTORY & fd.dwFileAttributes) )
			{
				MapFile file;
				if( file.Open(fd.cFileName, false) )
				{
					fd.cFileName[strlen(fd.cFileName) - 4] = 0; // throw off the extension .map

					LVITEM lvi = {0};
					lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
					lvi.pszText = fd.cFileName;
					lvi.state     = LVIS_SELECTED | LVIS_FOCUSED;
					lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

					lvi.iItem = ListView_GetItemCount(hwndLV);
					int item = ListView_InsertItem(hwndLV, &lvi);

					if( 0 == _stricmp(g_options.cMapName, fd.cFileName) )
						last_map_index = item;

					char size[64];
					int h = 0, w = 0;
					file.getMapAttribute("width", w);
					file.getMapAttribute("height", h);
					wsprintf(size, "%d*%d", w, h);
					ListView_SetItemText(hwndLV, item, 1, size);

					string_t theme_name;
					file.getMapAttribute("theme", theme_name);
					ListView_SetItemText(hwndLV, item, 2, const_cast<char*>(theme_name.c_str()));
				}
			}
		} while( FindNextFile(hSearch, &fd) );

		FindClose(hSearch);
		SetCurrentDirectory("..");


		//
		// выбираем последнюю карту
		//

		SetFocus(hwndLV);
		ListView_SetItemState(hwndLV, last_map_index, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
        ListView_SetSelectionMark(hwndLV, last_map_index);
		int scroll = 0;
		RECT rt;
		for( int i = 0; i < last_map_index; i++ )
		{
			ListView_GetSubItemRect(hwndLV, 0, 0, LVIR_BOUNDS, &rt);
			scroll += rt.bottom - rt.top;
		}
		GetClientRect(hwndLV, &rt);
		scroll -= (rt.bottom - rt.top) >> 1;
		ListView_Scroll(hwndLV, 0, scroll);



		//
		// установка опций
		//

		SETCHECK(IDC_ALLOW_EDITOR, g_options.bAllowEditor);
		SETCHECK(IDC_ALLOW_EDITPLAYERS, g_options.bAllowPlayerChange);
		SETCHECK(IDC_NIGHTMODE, g_options.bNightMode);

		SendDlgItemMessage(hDlg, IDC_SPIN_SPEED, UDM_SETRANGE, 0, MAKELONG(MAX_GAMESPEED, MIN_GAMESPEED));
		SetDlgItemInt(hDlg, IDC_SPEED, __max(MIN_GAMESPEED, __min(MAX_GAMESPEED, OPT(gameSpeed))), FALSE);
		SendDlgItemMessage(hDlg, IDC_SPIN_FL, UDM_SETRANGE, 0, MAKELONG((short) MAX_FRAGLIMIT, (short) 0));
		SetDlgItemInt(hDlg, IDC_FRAGLIMIT, __max(0, __min(MAX_FRAGLIMIT, g_options.fraglimit)), FALSE);
		SendDlgItemMessage(hDlg, IDC_SPIN_TL, UDM_SETRANGE, 0, MAKELONG((short) MAX_TIMELIMIT, (short) 0));
		SetDlgItemInt(hDlg, IDC_TIMELIMIT, __max(0, __min(MAX_TIMELIMIT, g_options.timelimit)), FALSE);


		//
		// установка крупного шрифта в заголовке
		//

		LOGFONT lf = {0};
		lf.lfHeight    = 28;
		lf.lfWeight    = FW_BLACK;
		lf.lfUnderline = TRUE;
		lf.lfCharSet = RUSSIAN_CHARSET;
		strcpy(lf.lfFaceName, "Arial");
		hFont = CreateFontIndirect(&lf);
		SendDlgItemMessage(hDlg, IDC_TITLE, WM_SETFONT, (WPARAM) hFont, 1);


		//
		// настройка списка игроков
		//

		hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);

		ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);
		ListView_SetBkColor(hwndLV, ICOLOR_BACKGROUND);

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;

		lvc.cx = 170;
		lvc.iSubItem = 0;
		lvc.pszText = "Имя игрока";
		ListView_InsertColumn(hwndLV, lvc.iSubItem, &lvc);

		lvc.cx = 80;
		lvc.iSubItem = 1;
		lvc.pszText = "Скин";
		ListView_InsertColumn(hwndLV, lvc.iSubItem, &lvc);

		lvc.cx = 80;
		lvc.iSubItem = 2;
		lvc.pszText = "Класс";
		ListView_InsertColumn(hwndLV, lvc.iSubItem, &lvc);

//		lvc.cx = 80;
//		lvc.iSubItem = 2;
//		lvc.pszText = "Тип";
//		ListView_InsertColumn(hwndLV, lvc.iSubItem, &lvc);

		lvc.cx = 80;
		lvc.iSubItem = 3;
		lvc.pszText = "Команда";
		ListView_InsertColumn(hwndLV, lvc.iSubItem, &lvc);

		dmUpdateUI(hDlg);
		SetFocus(GetDlgItem(hDlg, IDC_MAPLIST));
	} return FALSE;
	case WM_DESTROY:
		SendDlgItemMessage(hDlg, IDC_TITLE, WM_SETFONT, NULL, 1);
		DeleteObject(hFont);
		break;
	}
    return FALSE;
}
*/

DWORD plCheckPlayers(GC_Player *pExcept)
{
	DWORD dwHasPlayers = 0;

	ENUM_BEGIN(players, GC_Player, pPlayer)
	{
		if (pExcept == pPlayer || pPlayer->IsKilled()) continue;
		if (pPlayer->_nIndex < MAX_HUMANS)
			dwHasPlayers |= (1 << pPlayer->_nIndex);
	} ENUM_END();

	return dwHasPlayers;
}

/*
LRESULT CALLBACK dlgPlayers(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	PLAYERDESCEX pdex;
	static HFONT hFont;

	int wmId, wmEvent;
	switch( message )
	{
//	IMPLEMENT_EASYMOVE(hDlg, wParam, lParam);
//	IMPLEMENT_APPEARENCE(hDlg);
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
		case NM_SETFOCUS :
			SendMessage(hDlg, DM_SETDEFID, IDC_EDIT, 0);
			break;
//		case NM_CUSTOMDRAW:
//			SetWindowLong(hDlg, DWL_MSGRESULT, OnCustomDraw((LPNMLVCUSTOMDRAW) lParam));
//			return TRUE;
		}
	} break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
		{
		case BN_SETFOCUS:
			SendMessage(hDlg, DM_SETDEFID, wmId, 0);
			break;
		case BN_CLICKED:
			switch (wmId)
			{
			case IDC_ADD:
			{
				_ASSERT(g_level);
				_ASSERT(g_options.dm_nPlayers < MAX_PLAYERS);
				ZeroMemory(&pdex, sizeof(PLAYERDESCEX));

				std::vector<string_t> names;
				g_texman->GetTextureNames(names, "skin/");
				strcpy(pdex.skin, names[rand() % names.size()].c_str());
				strcpy(pdex.name, "new player");

				pdex.dwHasPlayers = plCheckPlayers(NULL);
				pdex.type = -1;

				LOGOUT_1("DialogBox(IDD_ADDPLAYER)\n");
				if (IDOK == DialogBoxParam(g_hInstance, (LPCTSTR)IDD_ADDPLAYER, hDlg,
					(DLGPROC) dlgAddPlayer, (LPARAM) &pdex) )
				{
					GC_Player *pPlayer = new GC_Player(0);
					strcpy(pPlayer->_name, pdex.name);
					strcpy(pPlayer->_skin, pdex.skin);
					pPlayer->UpdateSkin();
					pPlayer->SetController( pdex.type );
					pPlayer->_team = pdex.team;

					AddPlayerToListView(GetDlgItem(hDlg, IDC_PLRLIST), pPlayer->_name,
						pPlayer->_skin, pPlayer->_class, pPlayer->_nIndex, pPlayer->_team, pPlayer);

					EnableWindow(GetDlgItem(hDlg, IDC_PLRLIST), TRUE);
				}
			} break;
			case IDC_EDIT:
			{
				ZeroMemory(&pdex, sizeof(PLAYERDESCEX));

				HWND hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);
				int index = ListView_GetSelectionMark(hwndLV);
				if (-1 != index)
				{
					memcpy(&pdex, &g_options.dm_pdPlayers[index], sizeof(PLAYERDESC));

					LVITEM lvi = {0};
					lvi.iItem = index;
					lvi.iSubItem = 0;
					lvi.mask = LVIF_PARAM;

					ListView_GetItem(hwndLV, &lvi);
					_ASSERT(lvi.lParam);

					GC_Player *pPlayer = (GC_Player *) lvi.lParam;

					pdex.dwHasPlayers = plCheckPlayers(pPlayer);

					strcpy(pdex.name, pPlayer->_name);
					strcpy(pdex.skin, pPlayer->_skin);
					pdex.type = pPlayer->_nIndex;
					pdex.team = pPlayer->_team;

					LOGOUT_1("DialogBox(IDD_ADDPLAYER)\n");
					if (IDOK == DialogBoxParam(g_hInstance, (LPCTSTR)IDD_ADDPLAYER,
						hDlg, (DLGPROC) dlgAddPlayer, (LPARAM) &pdex))
					{
						strcpy(pPlayer->_name, pdex.name);
						strcpy(pPlayer->_skin, pdex.skin);
						pPlayer->UpdateSkin();
						pPlayer->SetController( pdex.type );
						pPlayer->_team = pdex.team;

						AddPlayerToListView(GetDlgItem(hDlg, IDC_PLRLIST), pPlayer->_name,
							pPlayer->_skin, pPlayer->_class, pPlayer->_nIndex, pPlayer->_team, pPlayer, index);
					}
				}
			} break;
			case IDC_KICK:
			{
				HWND hwndLV = GetDlgItem(hDlg, IDC_PLRLIST);
				int index = ListView_GetSelectionMark(hwndLV);
				if (-1 != index && IDYES == MessageBoxT(hDlg, "Удалить игрока?", MB_YESNO|MB_ICONQUESTION))
				{
					LVITEM lvi = {0};
					lvi.iItem = index;
					lvi.iSubItem = 0;
					lvi.mask = LVIF_PARAM;

					ListView_GetItem(hwndLV, &lvi);
					_ASSERT(lvi.lParam);

					((GC_Player *) lvi.lParam)->Kill();
					ListView_DeleteItem(hwndLV, index);

					if (0 == ListView_GetItemCount(hwndLV))
					{
						EnableWindow(hwndLV, FALSE);
						SetFocus(GetDlgItem(hDlg, IDC_ADD));
					}
				}
			} break;
			case IDCANCEL:
				EndDialog(hDlg, wmId);
				break;
			}
			break;
		}
		break;

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
		lvc.iSubItem = 1;
		lvc.pszText = "Скин";
		ListView_InsertColumn(hwndLV, 1, &lvc);

		lvc.cx = 80;
		lvc.iSubItem = 2;
		lvc.pszText = "Тип";
		ListView_InsertColumn(hwndLV, 2, &lvc);

		lvc.cx = 80;
		lvc.iSubItem = 3;
		lvc.pszText = "Команда";
		ListView_InsertColumn(hwndLV, 3, &lvc);


		// заполняем список игроков
		if( g_level )
		{
			ENUM_BEGIN(players, GC_Player, pPlayer)
			{
				if (pPlayer->IsKilled()) continue;
				AddPlayerToListView( hwndLV, pPlayer->_name, pPlayer->_skin, pPlayer->_class,
					pPlayer->_nIndex, pPlayer->_team,	pPlayer );
			} ENUM_END();
		}

		EnableWindow(hwndLV, 0 != ListView_GetItemCount(hwndLV));
	} return TRUE;
	case WM_DESTROY:
		SendDlgItemMessage(hDlg, IDC_TITLE, WM_SETFONT, NULL, 1);
		DeleteObject(hFont);
		break;
	}
    return FALSE;
}
*/

/*
void SetVolume(LONG lVolume)
{
	g_conf.s_volume->SetInt(lVolume);
	if( !g_level ) return;

	// после изменения g_options.dwVolume необходимо
	// вызвать UpdateVolume() для всех объектов GC_Sound

	ENUM_BEGIN(sounds, GC_Sound, pSound) {
		pSound->UpdateVolume();
	} ENUM_END();
}
*/


LRESULT CALLBACK dlgOptions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	static HFONT hFont;
	static LONG  lInitVolume;	// в случае отмены восстанавливаем значение громкости

	switch (message)
	{
//	IMPLEMENT_EASYMOVE(hDlg, wParam, lParam);
//	IMPLEMENT_APPEARENCE(hDlg);
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
			case IDC_SETTINGS_PLAYER1:
			case IDC_SETTINGS_PLAYER2:
			case IDC_SETTINGS_PLAYER3:
			case IDC_SETTINGS_PLAYER4:
			case IDC_SETTINGS_WINAMP:
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
			case IDC_ACC_0:
			case IDC_ACC_1:
			case IDC_ACC_2:
			case IDC_ACC_3:
			case IDC_ACC_4:
				g_options.nAIAccuracy = wmId - IDC_ACC_0;
				break;
			case IDOK:
				g_options.bParticles      = GETCHECK(IDC_CHK_PARTICLES);
				g_options.bDamageLabel    = GETCHECK(IDC_CHK_DAMLABEL);
				g_options.bShowFPS        = GETCHECK(IDC_CHK_FPS);
				g_options.bShowTime       = GETCHECK(IDC_CHK_TIMER);
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
			case IDC_SETTINGS_PLAYER1:
			case IDC_SETTINGS_PLAYER2:
			case IDC_SETTINGS_PLAYER3:
			case IDC_SETTINGS_PLAYER4:
			{
				int index = wmId - IDC_SETTINGS_PLAYER1;

				DialogBoxParam(g_hInstance, (LPCTSTR)IDD_DIALOG_CONTROLS, hDlg,
					(DLGPROC) dlgControls, (LPARAM) &g_options.players[index]);

				FreeDirectInput();
				InitDirectInput(g_env.hMainWnd);

				if (g_level)
				{
					ENUM_BEGIN(players, GC_Player, pPlayer) {
						if ( pPlayer->_nIndex == index ) pPlayer->SetController(index);
					} ENUM_END();
				}
			} break;
			case IDC_SETTINGS_WINAMP:
				DialogBox(g_hInstance, (LPCTSTR)IDD_WINAMP_CONTROL, hDlg, (DLGPROC) dlgWinampControl);
				FreeDirectInput();
				InitDirectInput(g_env.hMainWnd);
				break;
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

		SETCHECK(IDC_CHK_PARTICLES, g_options.bParticles);
		SETCHECK(IDC_CHK_DAMLABEL,  g_options.bDamageLabel);
		SETCHECK(IDC_CHK_FPS,       g_options.bShowFPS);
		SETCHECK(IDC_CHK_TIMER,     g_options.bShowTime);
		SETCHECK(IDC_CHK_SHOWSELECTMODE, g_conf.r_askformode->Get());

		SETCHECK(IDC_ACC_0 + g_options.nAIAccuracy, TRUE);

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

void mainUpdateUI(HWND hDlg)
{
//	if (g_level)
//	{
//		switch (g_options.gameType)
//		{
//		case GT_EDITOR:
//			SetDlgItemText(hDlg, IDC_INTRO, "Редактор карт");
//			break;
//		case GT_DEATHMATCH:
//			SetDlgItemText(hDlg, IDC_INTRO, "Deathmatch (каждый сам за себя)");
//			break;
//		}
//	}
//	else
		SetDlgItemText(hDlg, IDC_INTRO, TXT_VERSION);


	EnableWindow(GetDlgItem(hDlg, IDCANCEL   ), NULL != g_level);
	EnableWindow(GetDlgItem(hDlg, IDC_SAVE   ), NULL != g_level);
	EnableWindow(GetDlgItem(hDlg, IDC_PLAYERS), NULL != g_level && OPT(bAllowPlayerChange));
}


/*
LRESULT CALLBACK dlgMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HMENU _hMenu;

	int wmId, wmEvent;


	switch( message )
	{
//	IMPLEMENT_EASYMOVE(hDlg, wParam, lParam);
//	IMPLEMENT_APPEARENCE(hDlg);
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
		{
		case BN_SETFOCUS:
			SendMessage(hDlg, DM_SETDEFID, wmId, 0);
			break;
		case BN_CLICKED:
			switch (wmId)
			{
			case IDC_EXIT:
				if( IDYES == MessageBoxT(hDlg, "Выход в Windows?", MB_YESNO|MB_ICONQUESTION) )
					DestroyWindow(g_env.hMainWnd);
				break;
//			case IDC_HOTSEAT:
//				LOGOUT_1("DialogBox(IDD_NEWDM)\n");
//				if (IDOK == DialogBox(g_hInstance, (LPCTSTR)IDD_NEWDM, hDlg, (DLGPROC) dlgNewDM) )
//				{
//					EndDialog(hDlg, 0);
//				}
//				break;
			case IDC_CREATE:
				LOGOUT_1("DialogBox(IDD_CREATESERVER)\n");
				DialogBox(g_hInstance, (LPCTSTR)IDD_CREATESERVER, hDlg, (DLGPROC) dlgCreateServer);
				break;
			case IDC_CONNECT:
//				OnPrintScreen();
//				break;
				LOGOUT_1("DialogBox(IDD_CONNECT)\n");
				if (IDOK == DialogBox(g_hInstance, (LPCTSTR)IDD_CONNECT, hDlg, (DLGPROC) dlgConnect) )
				{
				}
				break;
			case IDC_PLAYERSETUP:
				PLAYERDESCEX pdex;
				memcpy(&pdex, &OPT(pdeLocalPlayer), sizeof(PLAYERDESCEX));
				pdex.dwHasPlayers = 0;

				LOGOUT_1("DialogBox(IDD_ADDPLAYER)\n");
				if (IDOK == DialogBoxParam(g_hInstance, (LPCTSTR)IDD_ADDPLAYER, hDlg,
					(DLGPROC) dlgAddPlayer, (LPARAM) &pdex) )
				{
					memcpy(&OPT(pdeLocalPlayer), &pdex, sizeof(PLAYERDESC));
				}

				break;
			case IDC_NEWGAME:
			{
//#ifdef _DEBUG
				RECT rt;
				GetWindowRect(GetDlgItem(hDlg, IDC_NEWGAME), &rt);
				TrackPopupMenu(GetSubMenu(_hMenu, 3), TPM_LEFTBUTTON, rt.right, rt.bottom, 0, hDlg, NULL);
//#else
//				PUSH(hDlg, IDC_HOTSEAT);
//#endif
			} break;
			case IDC_LOAD:
			{
				char FileName[MAX_PATH] = {0};
				if( Load(hDlg, FileName) )
				{
					SAFE_DELETE(g_level);
					g_level = new Level();

					if( g_level->init_load(FileName) )
					{
						EndDialog(hDlg, 0);
					}
					else
					{
						SAFE_DELETE(g_level);
						MessageBoxT(hDlg, "Не удалось загрузить игру", MB_ICONERROR|MB_OK);
						mainUpdateUI(hDlg);
					}
				}
			} break;
			case IDC_SAVE:
			{
				char FileName[MAX_PATH] = {0};
				if( Save(hDlg, FileName) )
				{
					if( !g_level->Serialize(FileName) )
						MessageBoxT(hDlg, "Не удалось сохранить игру", MB_ICONERROR|MB_OK);
					else
						MessageBoxT(hDlg, "Игра успешно сохранена", MB_ICONINFORMATION|MB_OK);
				}
			} break;
			//-----------------------------------------------------------------
			case IDC_EDITOR:
			{
				if( g_level )
				{
					EnableMenuItem(_hMenu, IDM_EXPORT,
						 OPT(gameType) == GT_EDITOR ? MF_ENABLED:MF_GRAYED);
					EnableMenuItem(_hMenu, IDM_MAPSETTINGS,
						(OPT(gameType)==GT_EDITOR||OPT(bModeEditor)) ? MF_ENABLED:MF_GRAYED);
				}
				else
				{
					EnableMenuItem(_hMenu, IDM_EXPORT, MF_GRAYED);
					EnableMenuItem(_hMenu, IDM_MAPSETTINGS, MF_GRAYED);
				}

				RECT rt;
				GetWindowRect(GetDlgItem(hDlg, IDC_EDITOR), &rt);
				TrackPopupMenu(GetSubMenu(_hMenu, 2), TPM_LEFTBUTTON, rt.right, rt.bottom, 0, hDlg, NULL);
			} break;
			case IDM_EMPTYMAP:
				if( IDOK == DialogBox(g_hInstance, (LPCTSTR)IDD_NEWMAP, hDlg, (DLGPROC) dlgNewMap) )
				{	
					SAFE_DELETE(g_level);
					g_level = new Level();
					g_level->Init(OPT(xsize), OPT(ysize));
                    if( !g_level->init_emptymap() )
					{
						SAFE_DELETE(g_level);
						MessageBoxT(hDlg, "Ошибка...", MB_ICONERROR);
					}

					g_gui->Show(false);
					EndDialog(hDlg, 0);
				}
				break;
			case IDM_MAPSETTINGS:
				LOGOUT_1("DialogBox(IDD_MAP_INFO)\n");
				DialogBox(g_hInstance, (LPCTSTR)IDD_MAP_SETTINGS, hDlg, (DLGPROC) dlgMapSettings);
				break;
			case IDM_EXPORT:
			{
				_ASSERT(g_level);
				char fn[MAX_PATH] = {0};
				if( Export(hDlg, fn) )
				{
					if( !g_level->Export(fn) )
					{
						MessageBoxT(hDlg, "Ошибка", MB_ICONERROR|MB_OK);
					}
					else
					{
						char *name = strrchr(fn, '\\'); // throw off the path
						name[strlen(name) - 4] = 0;     // throw off the extension .map
						if(name) strcpy(g_options.cMapName, name+1);
						MessageBoxT(hDlg, "Успешно", MB_ICONINFORMATION|MB_OK);
					}
				}
			} break;
//			case IDM_IMPORT:
//			{
//				char FileName[MAX_PATH] = {0};
//				if( Import(hDlg, FileName) )
//				{
//					if ( LEVEL_INIT_PARAM(init_import_and_edit, FileName) )
//					{
//						EndDialog(hDlg, 0);
//					}
//					else
//					{
//						MessageBoxT(hDlg, "Не удалось импортировать карту", MB_ICONERROR|MB_OK);
//						mainUpdateUI(hDlg);
//					}
//				}
//			} break;
			//-----------------------------------------------------------------
			case IDC_OPTIONS:
				LOGOUT_1("DialogBox(IDD_OPTIONS)\n");
				DialogBox(g_hInstance, (LPCTSTR)IDD_OPTIONS, hDlg, (DLGPROC) dlgOptions);
				break;
			case IDC_PLAYERS:
				LOGOUT_1("DialogBox(IDD_PLAYERS)\n");
				_ASSERT(g_level);
				DialogBox(g_hInstance, (LPCTSTR)IDD_PLAYERS, hDlg, (DLGPROC) dlgPlayers);
				break;
			case IDCANCEL:
				if( g_level )
				{
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				}
				break;
			}
			break;
		}
		break;
	case WM_INITDIALOG:
		_hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDC_TANK));
		mainUpdateUI(hDlg);
		return TRUE;
	case WM_DESTROY:
		DestroyMenu(_hMenu);
		break;
	}
    return FALSE;
}
*/

LRESULT CALLBACK dlgSelectObject(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
			g_options.nCurrentObject = SendDlgItemMessage(hDlg, IDC_OBJECTLIST, LB_GETCURSEL,0,0);
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
			break;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return FALSE;
			break;
		}
		break;

	case WM_INITDIALOG:
		for( int i = 0; i < _Editor::Inst()->GetObjectCount(); ++i )
			SendDlgItemMessage(hDlg, IDC_OBJECTLIST, LB_ADDSTRING, 0,
			(LPARAM) _Editor::Inst()->GetDesc(i));
		SendDlgItemMessage(hDlg, IDC_OBJECTLIST, LB_SETCURSEL, g_options.nCurrentObject, 0);
		return TRUE;
	}
    return FALSE;
}

LRESULT CALLBACK dlgMapSettings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (message)
	{
//		IMPLEMENT_EASYMOVE(hDlg, wParam, lParam);
//		IMPLEMENT_APPEARENCE(hDlg);
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


void Controls_ListViewUpdate(HWND hwndLV, LPPLAYER pl, BOOL bErase)
{
	if (bErase) ListView_DeleteAllItems(hwndLV);

	LVITEM lvi = {0};
	lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
	lvi.state     = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.iItem = ListView_GetItemCount(hwndLV);

#define INSERT_ITEM(key, name){						\
	char s[40] = {0};								\
	GetKeyName(pl->KeyMap.key, s);					\
	lvi.pszText = name;								\
	lvi.lParam = (LPARAM) &pl->KeyMap.key;			\
	if (bErase) ListView_InsertItem(hwndLV, &lvi);	\
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
	INSERT_ITEM(keyForvard,		"Вперед");

#undef INSERT_ITEM
}

void EditKey(HWND hwndLV, LPPLAYER pl)
{
	if (GetFocus() != hwndLV) return;

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
	static LPPLAYER pl;
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
			if ( GETCHECK(IDC_RADIO_MOUSE    ) ) pl->ControlType = CT_USER_MOUSE;
			if ( GETCHECK(IDC_RADIO_MOUSE2   ) ) pl->ControlType = CT_USER_MOUSE2;
			if ( GETCHECK(IDC_RADIO_KEYBOARD ) ) pl->ControlType = CT_USER_KB;
			if ( GETCHECK(IDC_RADIO_KEYBOARD2) ) pl->ControlType = CT_USER_KB2;
			if ( GETCHECK(IDC_RADIO_HYBRID   ) ) pl->ControlType = CT_USER_HYBRID;
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
		pl = (LPPLAYER)lParam;

		SETCHECK(IDC_RADIO_KEYBOARD,  pl->ControlType == CT_USER_KB);
		SETCHECK(IDC_RADIO_KEYBOARD2, pl->ControlType == CT_USER_KB2);
		SETCHECK(IDC_RADIO_MOUSE,     pl->ControlType == CT_USER_MOUSE);
		SETCHECK(IDC_RADIO_MOUSE2,    pl->ControlType == CT_USER_MOUSE2);
		SETCHECK(IDC_RADIO_HYBRID,    pl->ControlType == CT_USER_HYBRID);


		hwndLV = GetDlgItem(hDlg, IDC_LIST);

		ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);
		ListView_SetBkColor(hwndLV, ICOLOR_BACKGROUND);

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


void Winamp_ListViewUpdate(HWND hwndLV, BOOL bErase)
{
	if (bErase) ListView_DeleteAllItems(hwndLV);

	LVITEM lvi = {0};
	lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
	lvi.state     = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.stateMask = lvi.state;

#define INSERT_ITEM(key, name){								\
	char s[40] = {0};										\
	GetKeyName(g_options.wkWinampKeys.key, s);			\
	lvi.pszText = name;										\
	lvi.lParam = (LPARAM) &g_options.wkWinampKeys.key;	\
	if (bErase) ListView_InsertItem(hwndLV, &lvi);			\
	else lvi.iItem--;										\
	ListView_SetItemText(hwndLV, lvi.iItem, 1, s);}

	INSERT_ITEM(keyRew5s,		"Назад на 5 секунд");
	INSERT_ITEM(keyFfwd5s,		"Вперед на 5 секунд");
	INSERT_ITEM(keyVolumeDown,	"Уменьшить громкость");
	INSERT_ITEM(keyVolumeUp,	"Увеличить громкость");
	INSERT_ITEM(keyButton5,		"Следующий трек");
	INSERT_ITEM(keyButton4,		"Стоп");
	INSERT_ITEM(keyButton3,		"Пауза");
	INSERT_ITEM(keyButton2,		"Воспроизведение");
	INSERT_ITEM(keyButton1,		"Предыдущий трек");

#undef INSERT_ITEM
}

void EditKey(HWND hwndLV)
{
	if (GetFocus() != hwndLV) return;

	LVITEM lvi = {0};
	lvi.mask =   LVIF_PARAM;
	lvi.iItem    = ListView_GetSelectionMark(hwndLV);
	ListView_GetItem(hwndLV, &lvi);
	DialogBoxParam(g_hInstance, (LPCTSTR) IDD_DIALOG_GETKEY,
		GetParent(hwndLV), (DLGPROC) dlgGetKey, lvi.lParam);
	Winamp_ListViewUpdate(hwndLV, FALSE);
}

LRESULT CALLBACK dlgWinampControl(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndLV;
	LVITEM lvi = {0};

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
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		case IDOK:
			EditKey(hwndLV);
			break;
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code)
		{
		case LVN_ITEMACTIVATE:
			EditKey(hwndLV);
			break;
//		case NM_CUSTOMDRAW:
//			SetWindowLong(hDlg, DWL_MSGRESULT, OnCustomDraw((LPNMLVCUSTOMDRAW) lParam));
//			return TRUE;
//			break;
		}
		break;

	case WM_INITDIALOG:
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

		Winamp_ListViewUpdate(hwndLV, TRUE);
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
*/

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
