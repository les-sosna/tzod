// interface.h

#pragma once


int MessageBoxT(HWND hWnd, LPCTSTR lpText, UINT uType);

LRESULT CALLBACK WndProc             (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK dlgDisplaySettings  (HWND, UINT, WPARAM, LPARAM);

///////////////////////////////////////////////////////////////////////////////
// end of file
