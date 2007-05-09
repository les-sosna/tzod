// interface.h

#pragma once


void MyRegisterClass(HINSTANCE hInstance);
//HWND InitInstance(HINSTANCE hInstance, int nCmdShow);


//void OnDrawCB(HWND hDlg, LPDRAWITEMSTRUCT pdi);
//void OnDrawLB(HWND hDlg, LPDRAWITEMSTRUCT pdi);
//void OnDrawButton(LPDRAWITEMSTRUCT pdi);
//void OnPaintBorder(HWND hWnd);
//LRESULT OnColorStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int MessageBoxT(HWND hWnd, LPCTSTR lpText, UINT uType);


#define WM_UPDATE_DATA	(WM_USER + 1024)


LRESULT CALLBACK WndProc             (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PropGridProc        (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PropGridIntProc     (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgSelectMode       (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgDisplaySettings  (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgAddPlayer        (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgOptions          (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgMain             (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgObjectProperties (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgSelectObject     (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgMapSettings      (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgGetKey           (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgWinampControl    (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgControls         (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgOtherDt          (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgCreateServer     (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgConnect          (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgNetPlayers       (HWND, UINT, WPARAM, LPARAM);

///////////////////////////////////////////////////////////////////////////////
// end of file
