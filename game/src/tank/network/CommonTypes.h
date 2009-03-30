// CommonTypes.h

#pragma once

#include "Variant.h"

#define MAX_SRVNAME 16

struct GameInfo
{
	char exeVer[16];
	DWORD dwMapCRC32;
	unsigned long seed;
	char  cMapName[MAX_PATH];
	char  cServerName[MAX_SRVNAME];
	short server_fps;
	//	short latency;
	short timelimit;
	short fraglimit;
	bool  nightmode;
};

VARIANT_DECLARE_TYPE(GameInfo);

///////////////////////////////////////////////////////////////////////////////

struct PlayerDesc
{
	std::string nick;
	std::string skin;
	std::string cls;
	int  team;
	int  score;
};

VARIANT_DECLARE_TYPE(PlayerDesc);

///////////////////////////////////////////////////////////////////////////////

struct BotDesc : public PlayerDesc
{
	int level;
};

VARIANT_DECLARE_TYPE(BotDesc);

///////////////////////////////////////////////////////////////////////////////

struct PlayerDescEx : public PlayerDesc
{
	unsigned short id;
};

VARIANT_DECLARE_TYPE(PlayerDescEx);

///////////////////////////////////////////////////////////////////////////////

struct PlayerReady
{
	unsigned short id;
	bool ready;
};

VARIANT_DECLARE_TYPE(PlayerReady);

// end of file
