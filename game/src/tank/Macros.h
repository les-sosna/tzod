// Macros.h

#pragma once


//-----------------------------------------------------------------------------
// Miscellaneous helper functions
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#define SAFE_KILL(p)         { if(p) { (p)->Kill();    (p)=NULL; } }

//----------------------------------------------------------

#define FOREACH(ls, type, var)                                                 \
	if( type *var = (type *) (0xffffffff) )                                    \
	for( OBJECT_LIST::iterator var##iterator = g_level->ls.begin();            \
	         (var = static_cast<type *>(*var##iterator)),                      \
	         (var##iterator != g_level->ls.end());                             \
	    var##iterator++)

#define FOREACH_R(ls, type, var)                                               \
	if( type *var = (type *) (0xffffffff) )                                    \
	for( OBJECT_LIST::reverse_iterator var##iterator = g_level->ls.rbegin();   \
	      (var = static_cast<type *>(*var##iterator)),                         \
	      (var##iterator != g_level->ls.rend());                               \
	    var##iterator++ )

//--------------------------------------------

///////////////////////////////////////////////////////////////////////////////

#define DW(lo, hi) ( (0x0000ffff & (DWORD)(lo)) | ((DWORD) (hi) << 16) )
#define PUSH(dlg, id) PostMessage(dlg, WM_COMMAND, DW(id, BN_CLICKED), (LPARAM) GetDlgItem(dlg, id))

#define GET_DLG_ITEM_TEXT(hdlg, id, str)      \
{                                             \
	HWND item = GetDlgItem(hdlg, id);         \
	_ASSERT(item);                            \
	int len = 1+GetWindowTextLength(item);    \
	str.resize(len);                          \
	GetWindowText(item, &str[0], len);        \
	str.resize(len-1);                        \
}

///////////////////////////////////////////////////////////////////////////////

#define WIDTH(rect) (rect.right - rect.left)
#define HEIGHT(rect) (rect.bottom - rect.top)

///////////////////////////////////////////////////////////////////////////////

#define LEVEL_INIT_PARAM(init, param) (SAFE_DELETE(g_level), g_level = new Level(), \
	(g_level->init(param) ? TRUE:(SAFE_DELETE(g_level), FALSE)))

///////////////////////////////////////////////////////////////////////////////
// end of file
