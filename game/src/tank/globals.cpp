// globals.cpp

#include "stdafx.h"
#include "globals.h"

#include "fs/FileSystem.h"

ENVIRONMENT g_env;

LPDIRECTINPUTDEVICE8  g_pKeyboard = NULL;

CSoundManager   *g_pSoundManager = NULL;

HINSTANCE	 g_hInstance = NULL;

CSound      *g_pSounds[SND_COUNT] = {0};

IRender     *g_render  = NULL;
GuiManager  *g_gui     = NULL;
Level       *g_level   = NULL;

TextureManager *g_texman = NULL;

SafePtr<IFileSystem> g_fs;


// end of file
