// CommonTypes.h

#pragma once

#include "Variant.h"
#include "Level.h"

#define MAX_SRVNAME 16
#define MAX_MAPNAME 256

struct GameInfo
{
	char mapVer[16];
	unsigned long seed;
	char  cMapName[MAX_MAPNAME];
	char  cServerName[MAX_SRVNAME];
	short server_fps;
	//	short latency;
	short timelimit;
	short fraglimit;
	bool  nightmode;
};

VARIANT_DECLARE_TYPE(GameInfo);
VARIANT_DECLARE_TYPE(PlayerDesc);
VARIANT_DECLARE_TYPE(BotDesc);

///////////////////////////////////////////////////////////////////////////////

struct PlayerDescEx
{
	PlayerDesc pd;
	unsigned short idx;
};

VARIANT_DECLARE_TYPE(PlayerDescEx);

///////////////////////////////////////////////////////////////////////////////

struct PlayerReady
{
	unsigned short playerIdx;
	bool ready;
};

VARIANT_DECLARE_TYPE(PlayerReady);

// end of file
