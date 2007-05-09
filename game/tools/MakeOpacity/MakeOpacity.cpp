// MakeOpacity.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE g_hInst;								// current instance
COLORREF  g_background = 0x00ff0000;
BYTE      g_OpacityLevel = 128;
HBITMAP   g_hSource = NULL;
HBITMAP   g_hOutput = NULL;

BITMAPFILEHEADER g_bmfh	= {0};
BITMAPINFOHEADER g_bmih	= {0};


TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];								// The title bar text

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	dlgSetLevel(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;
	
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_MAKEOPACITY, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	
	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}
	
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_MAKEOPACITY);
	
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	return msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	
	wcex.cbSize = sizeof(WNDCLASSEX); 
	
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_MAKEOPACITY);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= CreateSolidBrush(g_background);
	wcex.lpszMenuName	= (LPCSTR)IDC_MAKEOPACITY;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
	
	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	
	g_hInst = hInstance; // Store instance handle in our global variable
	
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	
	if (!hWnd)
	{
		return FALSE;
	}

	DragAcceptFiles(hWnd, TRUE);
	
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	
	return TRUE;
}



bool ReadInfo(char* cFileName)
{
	HANDLE	hBMP_File = CreateFile(	cFileName, 
		GENERIC_READ, 
		FILE_SHARE_READ, 
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);
	
	DWORD nBytesReaded = 0;
	
	ReadFile(hBMP_File, &g_bmfh, sizeof(g_bmfh), &nBytesReaded, NULL );
	ReadFile(hBMP_File, &g_bmih, sizeof(g_bmih), &nBytesReaded, NULL );
	
	if ( g_bmih.biBitCount != 24 ) 
	{
		CloseHandle(hBMP_File);
		return false;
	}
	
	CloseHandle(hBMP_File);
	return true;
}

BOOL LoadFile(char *filename)
{
	if (!ReadInfo(filename))
	{
		return FALSE;
	}
	else
	{
		DeleteObject(g_hSource);
		g_hSource = (HBITMAP) LoadImage(NULL, filename, IMAGE_BITMAP, 0,0, LR_LOADFROMFILE);
	}

	return TRUE;
}

void OnOpen(HWND hWnd)
{
	char filename[MAX_PATH] = {0};
	
	char filter  [128] = {0};
	wsprintf (filter, "%s%c%s%c", "Bitmaps (*.bmp)", 0, "*.bmp", 0);
	
	OPENFILENAME ofn = {0};
	
	ofn.lStructSize = sizeof (ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filter;
	ofn.lpstrDefExt = "bmp";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = "Choose the source bitmap";
	ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	
	if ( GetOpenFileName(&ofn) )
	{
		if (!LoadFile(filename))
			MessageBox(hWnd, "Error!", szTitle, MB_ICONHAND);
		else
			InvalidateRect(hWnd, NULL, TRUE);
	}
}

void OnSaveAs(HWND hWnd)
{
	char filename[MAX_PATH] = {0};
	
	char filter  [128] = {0};
	wsprintf (filter, "%s%c%s%c", "Bitmaps (*.bmp)", 0, "*.bmp", 0);
	
	OPENFILENAME ofn = {0};
	
	ofn.lStructSize = sizeof (ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filter;
	ofn.lpstrDefExt = "bmp";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = "Choose the output bitmap";
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if ( GetSaveFileName(&ofn) )
	{
		BITMAP bmp;
		GetObject(g_hOutput, sizeof(BITMAP), &bmp);

		RGBTRIPLE *buffer = new RGBTRIPLE[bmp.bmHeight * bmp.bmWidth];


		HDC h_wnd_dc = GetWindowDC(hWnd);

		HDC h_out_dc = CreateCompatibleDC(h_wnd_dc);
		HBITMAP old_out = (HBITMAP) SelectObject(h_out_dc, g_hOutput);

		int t = 0;
		for (int y = bmp.bmHeight - 1; y >= 0; y--)
		for (int x = 0; x < bmp.bmWidth; x++)
		{
			COLORREF src = GetPixel(h_out_dc, x, y);
			buffer[t].rgbtBlue = (BYTE) ((src & 0x00ff0000) >> 16);
			buffer[t].rgbtGreen = (BYTE) ((src & 0x0000ff00) >> 8);
			buffer[t].rgbtRed = (BYTE) (src & 0x000000ff);
			t++;
		}
		
		SelectObject(h_out_dc, old_out);
		DeleteDC(h_out_dc);

		ReleaseDC(hWnd, h_wnd_dc);



		HANDLE	hBMP_File = CreateFile(	filename, 
										GENERIC_WRITE, 
										FILE_SHARE_READ, 
										NULL, 
										CREATE_ALWAYS, 
										FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
										NULL);
		DWORD nBytesWrited = 0;
		
		WriteFile(hBMP_File, &g_bmfh, sizeof(g_bmfh), &nBytesWrited, NULL );
		WriteFile(hBMP_File, &g_bmih, sizeof(g_bmih), &nBytesWrited, NULL );

		SetFilePointer(hBMP_File, g_bmfh.bfOffBits, NULL, FILE_BEGIN);
		WriteFile(hBMP_File, buffer, g_bmfh.bfSize - g_bmfh.bfOffBits, &nBytesWrited, NULL );

		CloseHandle(hBMP_File);

		delete[] buffer;
	}
}

void OnBackground(HWND hWnd)
{
	CHOOSECOLOR cc = {0};

	COLORREF custom[16] = {0};

	cc.lStructSize  = sizeof(CHOOSECOLOR);
	cc.hwndOwner    = hWnd;
	cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
	cc.rgbResult    = g_background;
	cc.lpCustColors = custom;

	if ( ChooseColor(&cc) )
	{
		g_background = cc.rgbResult;
		HBRUSH hOldBrush = (HBRUSH) GetClassLong(hWnd, GCL_HBRBACKGROUND);
		SetClassLong(hWnd, GCL_HBRBACKGROUND, (LONG) CreateSolidBrush(g_background));
		DeleteObject(hOldBrush);
		InvalidateRect(hWnd, NULL, TRUE);
	}
}


void OnSetLevel(HWND hWnd)
{
	HDC h_wnd_dc = GetWindowDC(hWnd);

	HDC h_src_dc = CreateCompatibleDC(h_wnd_dc);
	HBITMAP old_src = (HBITMAP) SelectObject(h_src_dc, g_hSource);

	BITMAP bmp;
	GetObject(g_hSource, sizeof(BITMAP), &bmp);

	DeleteObject(g_hOutput);
	g_hOutput = CreateCompatibleBitmap(h_src_dc, bmp.bmWidth, bmp.bmHeight);
	HDC h_out_dc = CreateCompatibleDC(h_wnd_dc);
	HBITMAP old_out = (HBITMAP) SelectObject(h_out_dc, g_hOutput);

	for (int x = 0; x < bmp.bmWidth; x++)
	for (int y = 0; y < bmp.bmHeight; y++)
	{
		COLORREF src = GetPixel(h_src_dc, x, y);

		RGBTRIPLE *rgb = (RGBTRIPLE *)(((BYTE*) &src) + 1);

		if (rgb->rgbtRed + rgb->rgbtGreen + rgb->rgbtBlue > 3 * rand() * g_OpacityLevel / RAND_MAX)
			SetPixel(h_out_dc, x, y, src);
		else
			SetPixel(h_out_dc, x, y, g_background);
	}
	
	SelectObject(h_out_dc, old_out);
	DeleteDC(h_out_dc);

	SelectObject(h_src_dc, old_src);
	DeleteDC(h_src_dc);

	ReleaseDC(hWnd, h_wnd_dc);

	
	InvalidateRect(hWnd, NULL, TRUE);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR szHello[MAX_LOADSTRING];
	LoadString(g_hInst, IDS_HELLO, szHello, MAX_LOADSTRING);
	
	switch (message) 
	{
	case WM_DROPFILES:
		HDROP hDrop;
		char filename[MAX_PATH];
		hDrop = (HDROP) wParam;
		DragQueryFile(hDrop, 0, filename, MAX_PATH);
		DragFinish(hDrop);
		if (!LoadFile(filename))
			MessageBox(hWnd, "Error!", szTitle, MB_ICONHAND);
		else
			InvalidateRect(hWnd, NULL, TRUE);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_OPEN:
			OnOpen(hWnd);
			break;
		case IDM_SAVEAS:
			OnSaveAs(hWnd);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_BACKGROUND:
			OnBackground(hWnd);
			break;
		case IDM_SETLEVEL:
			if (IDOK == DialogBox(g_hInst, (LPCTSTR)IDD_SETLEVEL, hWnd, (DLGPROC)dlgSetLevel))
			{
				OnSetLevel(hWnd);
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		RECT rt;
		GetClientRect(hWnd, &rt);
		if (g_hSource)
		{
			if (g_hOutput)
				DrawState(hdc, NULL, NULL, (LPARAM) g_hOutput, 0, 0, 0, 0, 0, DST_BITMAP);
			else
				DrawState(hdc, NULL, NULL, (LPARAM) g_hSource, 0, 0, 0, 0, 0, DST_BITMAP);
		}
		else
		{
			DrawText(hdc, szHello, strlen(szHello), &rt, DT_CENTER);
		}
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;
		
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
    return FALSE;
}



LRESULT CALLBACK dlgSetLevel(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemInt(hDlg, IDC_VALUE, g_OpacityLevel, FALSE);
		return TRUE;
		
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			if (LOWORD(wParam) == IDOK) g_OpacityLevel = GetDlgItemInt(hDlg, IDC_VALUE, NULL, FALSE);
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
    return FALSE;
}
