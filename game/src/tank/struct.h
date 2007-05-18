// struct.h

#pragma once

#include "script.h"
#include "constants.h"

//----------------------------------------------------------

typedef struct SAVEHEADER
{
	DWORD dwVersion;
	DWORD dwGameType;
	bool  nightmode;
	float timelimit;
	int   fraglimit;
	int   nObjects;
	float time;
	int   X, Y;	// размер уровня
	char  theme[MAX_PATH];
} SAVEHEADER, *LPSAVEHEADER;

//----------------------------------------------------------

struct WINAMPKEYS
{
	int keyButton1;
	int keyButton2;
	int keyButton3;
	int keyButton4;
	int keyButton5;
	int keyVolumeUp;
	int keyVolumeDown;
	int keyFfwd5s;
	int keyRew5s;
};

//----------------------------------------------------------

typedef struct VEHICLEKEYS
{
	int keyLeft;
	int keyRight;
	int keyForvard;
	int keyBack;
	int keyFire;
	int keyLight;
	int keyDrop;
	int keyTowerLeft;
	int keyTowerCenter;
	int keyTowerRight;
} VEHICLEKEYS, *LPVEHICLEKEYS;

//----------------------------------------------------------

enum enumControlType
{
	CT_USER_KB,
	CT_USER_KB2,
	CT_USER_MOUSE,
	CT_USER_MOUSE2,
	CT_USER_HYBRID,
};

//----------------------------------------------------------

typedef struct PLAYER
{
	VEHICLEKEYS     KeyMap;
	enumControlType ControlType;
	BOOL            bAI;
} PLAYER, *LPPLAYER;

//----------------------------------------------------------

#define MAX_PLRNAME	30
#define MAX_VEHCLSNAME	30

struct PlayerDesc
{
	char name[MAX_PLRNAME];
	char skin[MAX_PATH];
	char cls[MAX_VEHCLSNAME];
	int  team;
	union {
		short score;
		short type;
	};
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
    char  cMapName[MAX_PATH];
	char  cServerName[MAX_SRVNAME];
	short server_fps;
	short latency;
	short timelimit;
	short fraglimit;
	short seed;
	bool  nightmode;
} GAMEINFO, *LPGAMEINFO;

//----------------------------------------------------------


//----------------------------------------------------------
// параметры файлового диалога
typedef struct GETFILENAME
{
	DWORD   dwFlags;
	LPCTSTR lpszDirectory;
	LPCTSTR lpszActionName; // надпись на кнопке OK
	LPSTR   fileName;
	LPCTSTR lpszFileExt;    // расширение (например, "map")
	BOOL  (*lpValidateFunc)(LPCTSTR);
} GETFILENAME, *LPGETFILENAME;

#define GFNF_OVERWRITEPROMPT	1
#define GFNF_FILEMUSTEXIST		2

//----------------------------------------------------------
// состояние устройств ввода
typedef struct INPUTSTATE
{
	char	keys[256];
	int		mouse_x, mouse_y;
	int		mouse_wheel;
	bool	bLButtonState;
	bool	bRButtonState;
	bool	bMButtonState;
} INPUTSTATE, *LPINPUTSTATE;


//----------------------------------------------------------
// ресурсы интерфейса
/*
typedef struct INTERFACE_RESOURCES
{
	HBITMAP hbmBackground;
	HBITMAP hbmButton1;
	HBITMAP hbmButton2;
	HBITMAP hbmButton3;
	HBITMAP hbmCorner_lt;
	HBITMAP hbmCorner_rt;
	HBITMAP hbmCorner_lb;
	HBITMAP hbmCorner_rb;
	HBITMAP hbmBorder_t;
	HBITMAP hbmBorder_b;
	HBITMAP hbmBorder_l;
	HBITMAP hbmBorder_r;
	HBITMAP hbmItem;
	//-------------------
	HBRUSH hbrBackground;
	HBRUSH hbrBkColor;
	HBRUSH hbrNull;
	HBRUSH hbrItem;
	//-------------------
	HPEN hpSelection;
} INTERFACE_RESOURCES;
*/


//----------------------------------------------------------

typedef float AIPRIORITY;


///////////////////////////////////////////////////////////////////////////////
// end of file
