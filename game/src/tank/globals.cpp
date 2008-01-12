// globals.cpp

#include "stdafx.h"
#include "globals.h"

#include "fs/FileSystem.h"
#include "core/Console.h"

#include "KeyMapper.h"

ENVIRONMENT g_env;

HINSTANCE    g_hInstance = NULL;


LPDIRECTINPUTDEVICE8  g_pKeyboard = NULL;

CSoundManager   *g_soundManager = NULL;
MusicPlayer     *g_music         =  NULL;
CSound          *g_pSounds[SND_COUNT] = {NULL};

IRender     *g_render  = NULL;
GuiManager  *g_gui     = NULL;
Level       *g_level   = NULL;
TankServer  *g_server  = NULL;
TankClient  *g_client  = NULL;

TextureManager *g_texman = NULL;

SafePtr<IFileSystem> g_fs;

ConsoleBuffer *g_console = NULL;

KeyMapper *g_keys = NULL;

// end of file
