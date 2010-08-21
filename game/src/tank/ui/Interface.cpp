// interface.cpp
//

#include "stdafx.h"

#include "interface.h"
#include "macros.h"
#include "functions.h"

#include "level.h"

#include "directx.h"

#include "config/Config.h"

#include "video/RenderOpenGL.h"
#include "video/RenderDirect3D.h"
#include "video/TextureManager.h"

#include "core/debug.h"

#include "ui/GuiManager.h"
#include "ui/Window.h"

#include "network/TankClient.h"
#include "network/TankServer.h"

#include "fs/MapFile.h"

#include "gc/Player.h"
#include "gc/Sound.h"
#include "gc/GameClasses.h"
#include "gc/RigidBody.h"

#include "res/resource.h"

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
	case WM_ACTIVATE:
	{
		WORD wActive = LOWORD(wParam);
		WORD wMinimized = HIWORD(wParam);

		if( (wMinimized || (wActive == WA_INACTIVE)) && !g_env.minimized )
		{
			TRACE("main window deactivated");
			g_env.minimized = true;
			PauseGame(true);
		}
		else if( g_env.minimized )
		{
			TRACE("main window activated");
			g_env.minimized = false;
			PauseGame(false);
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
			if( g_texman )
			{
				g_texman->SetCanvasSize(g_render->GetWidth(), g_render->GetHeight());
			}
			if( g_gui )
			{
				g_gui->GetDesktop()->Resize((float) g_render->GetWidth(), (float) g_render->GetHeight());
			}
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
		if( g_gui ) g_gui->ProcessKeys(message, wParam);
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
		g_conf.r_fullscreen.Set( GETCHECK(IDC_FULLSCREEN) );
		int render = GETCHECK(IDC_RENDER_OPENGL) ? 0 : 1;
		if( render != g_conf.r_render.GetInt() )
		{
			SAFE_DELETE(g_render);
			g_conf.r_render.SetInt(render);
		}
	}
	else
	{
		SETCHECK(IDC_FULLSCREEN, g_conf.r_fullscreen.Get());
		SETCHECK(g_conf.r_render.GetInt() ? IDC_RENDER_DIRECT3D : IDC_RENDER_OPENGL, TRUE);
	}


	if( !g_render )
	{
		g_render = g_conf.r_render.GetInt() ? renderCreateDirect3D() : renderCreateOpenGL();
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
				if( it->first.Width == g_conf.r_width.GetInt() &&
					it->first.Height == g_conf.r_height.GetInt() )
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
					g_conf.r_width.SetInt( it->first.Width );
					g_conf.r_height.SetInt( it->first.Height );
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
				if( it->first.BitsPerPixel == g_conf.r_bpp.GetInt() )
					SendDlgItemMessage(hDlg, IDC_BPP, CB_SETCURSEL, index, 0);
			}
			else
			{
				char item_text[256] = {0};
				SendDlgItemMessage(hDlg, IDC_BPP, CB_GETLBTEXT,
					SendDlgItemMessage(hDlg,IDC_BPP,CB_GETCURSEL,0,0), (LPARAM)item_text);
				if( 0 == strcmp(item_text, buf) )
				{
					g_conf.r_bpp.SetInt(it->first.BitsPerPixel);
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
				if( it->first.RefreshRate == g_conf.r_freq.GetInt() )
					SendDlgItemMessage(hDlg, IDC_RATE, CB_SETCURSEL, index, 0);
			}
			else
			{
				char item_text[256] = {0};
				SendDlgItemMessage(hDlg, IDC_RATE, CB_GETLBTEXT,
					SendDlgItemMessage(hDlg,IDC_RATE,CB_GETCURSEL,0,0), (LPARAM)item_text);
				if( 0 == strcmp(item_text, buf) )
				{
					g_conf.r_freq.SetInt(it->first.RefreshRate);
				}
			}
		}
		if( -1 == SendDlgItemMessage(hDlg, IDC_RATE, CB_GETCURSEL, 0, 0) )
			SendDlgItemMessage(hDlg, IDC_RATE, CB_SETCURSEL, 0, 0);
	}

	EnableWindow(GetDlgItem(hDlg, IDC_BPP ), g_conf.r_fullscreen.Get() );
	EnableWindow(GetDlgItem(hDlg, IDC_RATE), g_conf.r_fullscreen.Get() );

	s_bUpdating = FALSE;
}

INT_PTR CALLBACK dlgDisplaySettings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch( wmEvent )
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


///////////////////////////////////////////////////////////////////////////////
// end of file
