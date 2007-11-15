// struct.h

#pragma once

#include "script.h"
#include "constants.h"

//----------------------------------------------------------

typedef struct VEHICLEKEYS
{
	int keyLeft;
	int keyRight;
	int keyForward;
	int keyBack;
	int keyFire;
	int keyLight;
	int keyDrop;
	int keyTowerLeft;
	int keyTowerCenter;
	int keyTowerRight;
} VEHICLEKEYS, *LPVEHICLEKEYS;

//----------------------------------------------------------


#define MAX_PLRNAME     30
#define MAX_VEHCLSNAME  30

struct PlayerDesc
{
	char nick[MAX_PLRNAME];
	char skin[MAX_PATH];
	char cls[MAX_VEHCLSNAME];
	int  team;
	int  score;
};

struct PlayerDescEx : public PlayerDesc
{
	union
	{
		DWORD dwHasPlayers; // флаги, означающие присутствие игроков в списке
		DWORD dwNetworkId;
	};
};

//----------------------------------------------------------

#define MAX_SRVNAME 16

typedef struct GAMEINFO
{
	DWORD dwVersion;
	DWORD dwMapCRC32;
	unsigned long seed;
	char  cMapName[MAX_PATH];
	char  cServerName[MAX_SRVNAME];
	short server_fps;
	short latency;
	short timelimit;
	short fraglimit;
	bool  nightmode;
} GAMEINFO, *LPGAMEINFO;


//----------------------------------------------------------

typedef float AIPRIORITY;


///////////////////////////////////////////////////////////////////////////////
// end of file
