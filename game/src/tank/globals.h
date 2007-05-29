// globals.h

#pragma once

// forward declarations
struct IRender;

class TextureManager;
class CSoundManager;
class CSound;
class GuiManager;
class Level;
class IFileSystem;
class ConsoleBuffer;
class Editor;

struct GAMEOPTIONS;
struct ENVIRONMENT;

#include "SoundTemplates.h" // FIXME!
#include "struct.h"         // FIXME!

// ------------------------

extern CSound *g_pSounds[SND_COUNT];

extern HINSTANCE    g_hInstance;
extern LPDIRECTINPUTDEVICE8 g_pKeyboard;

extern IRender        *g_render;
extern TextureManager *g_texman;
extern CSoundManager  *g_pSoundManager;
extern GuiManager     *g_gui;
extern Level          *g_level;
extern ConsoleBuffer  *g_console;
extern Editor         *g_editor;
extern SafePtr<IFileSystem> g_fs;

///////////////////////////////////////////////////////////////////////////////

struct ENVIRONMENT
{
	INPUTSTATE envInputs;

	script_h hScript;       // handle to the script engine

	int		 nNeedCursor;   // счетчик контроллеров, которым нужен курсор
	bool	 minimized;	    // признак свернутости главного окна.

	int		 camera_x;
	int		 camera_y;

	HWND     hMainWnd;		// handle to main application window
};

extern ENVIRONMENT g_env;

// end of file
