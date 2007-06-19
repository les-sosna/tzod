// interface.h

#pragma once


int MessageBoxT(HWND hWnd, LPCTSTR lpText, UINT uType);

#define WM_UPDATE_DATA	(WM_USER + 1024)


LRESULT CALLBACK WndProc             (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgSelectMode       (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgDisplaySettings  (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgOptions          (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgSelectObject     (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgMapSettings      (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgGetKey           (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgControls         (HWND, UINT, WPARAM, LPARAM);

///////////////////////////////////////////////////////////////////////////////
// end of file
