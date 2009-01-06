// globals.cpp

#include "stdafx.h"
#include "globals.h"

#include "Level.h"
#include "fs/FileSystem.h"
#include "core/Console.h"

#include "KeyMapper.h"

ENVIRONMENT g_env;

HINSTANCE    g_hInstance;


LPDIRECTINPUTDEVICE8  g_pKeyboard;

CSoundManager   *g_soundManager;
MusicPlayer     *g_music;
CSound          *g_pSounds[SND_COUNT];

AppBase     *g_app;
IRender     *g_render;
GuiManager  *g_gui;
TankServer  *g_server;
TankClient  *g_client;

TextureManager *g_texman;

SafePtr<Level> g_level;
SafePtr<IFileSystem> g_fs;

KeyMapper *g_keys;

// end of file
