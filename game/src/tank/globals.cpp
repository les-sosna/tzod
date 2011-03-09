// globals.cpp

#include "stdafx.h"
#include "globals.h"

#include "fs/FileSystem.h"
#include "sound/MusicPlayer.h"
#include "Level.h"

MD5 g_md5;

ENVIRONMENT g_env;

CSoundManager   *g_soundManager;
CSound          *g_pSounds[SND_COUNT];

AppBase     *g_app;
IRender     *g_render;
UI::LayoutManager  *g_gui;

TextureManager *g_texman;
ClientBase     *g_client;

std::auto_ptr<Level>     g_level;
SafePtr<MusicPlayer>     g_music;
SafePtr<FS::FileSystem>  g_fs;

// end of file
