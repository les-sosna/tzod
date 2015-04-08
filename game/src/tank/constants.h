// constants.h

#pragma once


#define SEVERITY_ERROR 1


#define MAX_GAMESPEED   200
#define MIN_GAMESPEED   20

#define MAX_TIMELIMIT   1000
#define MAX_FRAGLIMIT   10000

#define MAX_NETWORKSPEED    60
#define MIN_NETWORKSPEED    10

#define MAX_LATENCY         10
#define MIN_LATENCY         1



// default animation speed
#define ANIMATION_FPS        30.0f  // fps


// distance to the crosshair
#define CH_DISTANCE_NORMAL  200.0f  // normal
#define CH_DISTANCE_THIN    150.0f  // for minigun

//-----------------------------------------------------------------------------

#define AI_MAX_LEVEL   4U

//-----------------------------------------------------------------------------
// compatibility

#define TXT_VERSION      "Tank: Zone of Death (1.52.0)"
#define TXT_WNDCLASS     "TankMainWindow"

//-----------------------------------------------------------------------------

#define MAX_PLAYERS     32
#define MAX_HUMANS       4

#define MAX_DT           0.05f
#define MAX_DT_FIXED     0.02f


//-----------------------------------------------------------------------------
// file paths
#define DIR_SCRIPTS      "scripts"
#define DIR_SAVE         "save"
#define DIR_MAPS         "maps"
#define DIR_SKINS        "skins"
#define DIR_THEMES       "themes"
#define DIR_MUSIC        "music"
#define DIR_SOUND        "sounds"
#define DIR_SCREENSHOTS  "screenshots"

#define FILE_CONFIG      "config.cfg"
#define FILE_LANGUAGE    "data/lang.cfg"
#define FILE_TEXTURES    DIR_SCRIPTS"/textures.lua"
#define FILE_STARTUP     DIR_SCRIPTS"/init.lua"

///////////////////////////////////////////////////////////////////////////////
// end of file
