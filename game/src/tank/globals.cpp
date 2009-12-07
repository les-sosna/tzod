// globals.cpp

#include "stdafx.h"
#include "globals.h"

#include "Level.h"
#include "fs/FileSystem.h"
#include "sound/MusicPlayer.h"

MD5 g_md5;

ENVIRONMENT g_env;

CSoundManager   *g_soundManager;
CSound          *g_pSounds[SND_COUNT];

AppBase     *g_app;
IRender     *g_render;
UI::LayoutManager  *g_gui;
TankServer  *g_server;
TankClient  *g_client;

TextureManager *g_texman;

Level          *g_level;
SafePtr<MusicPlayer>     g_music;
SafePtr<FS::FileSystem>  g_fs;

// end of file
