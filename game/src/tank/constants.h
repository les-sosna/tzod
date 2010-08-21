// constants.h

#pragma once

//-----------------------------------------------------------------------------
// z-layers
enum enumZOrder
{
	Z_EDITOR,           // editor labels
	Z_WATER,            // water
	Z_GAUSS_RAY,        // gauss ray
	Z_WALLS,            // walls
	Z_FREE_ITEM,        // not picked up item
	Z_VEHICLES,         // vehicles
	Z_ATTACHED_ITEM,    // picked up items (weapon etc.)
	Z_PROJECTILE,       // flying projectiles
	Z_EXPLODE,          // explosions
	Z_VEHICLE_LABEL,    // vehicle labels (the crosshair, the health bar, etc.)
	Z_PARTICLE,         // particles, smoke
	Z_WOOD,             // wood
	//------------------//
	Z_COUNT,            // total number of z-layers
	//------------------//
	Z_NONE = 0x7FFFFFFF // not drawn
};


//-----------------------------------------------------------------------------
// score table settings
#define SCORE_POS_NUMBER     16
#define SCORE_POS_NAME       48
#define SCORE_POS_SCORE      16 // from the right side
#define SCORE_LIMITS_LEFT    64
#define SCORE_TIMELIMIT_TOP  16
#define SCORE_FRAGLIMIT_TOP  36
#define SCORE_NAMES_TOP      64
#define SCORE_ROW_HEIGHT     24

//-----------------------------------------------------------------------------
// world settings
#define LEVEL_MINSIZE   16
#define LEVEL_MAXSIZE   512
#define CELL_SIZE       32             // cell size in pixels
#define LOCATION_SIZE   (CELL_SIZE*4)  // location size in pixels (should be at least 
                                       // as large as the largest sprite object)

#define MAX_GAMESPEED   200
#define MIN_GAMESPEED   20

#define MAX_TIMELIMIT   1000
#define MAX_FRAGLIMIT   10000

#define MAX_NETWORKSPEED    60
#define MIN_NETWORKSPEED    10

#define MAX_LATENCY         10
#define MIN_LATENCY         1


//-----------------------------------------------------------------------------
// weapons settings
#define WEAP_MG_TIME_RELAX      3.0f    // minigun crosshair relax time

#define WEAP_RL_HOMING_FACTOR   600.0f  // rocket homing factor
#define WEAP_RL_HOMING_TIME     4.0f    // time to loose target

#define WEAP_BFG_TARGET_SPEED   500.0f
#define WEAP_BFG_HOMMING_FACTOR 500.0f  // the bfg core homming factor
#define WEAP_BFG_RADIUS         200.0f  // maximum radius of the bfg core damage

#define WEAP_RAM_PERCUSSION     8.0f

#define WEAP_ZIPPO_TIME_RELAX   1.0f


#define SPEED_ROCKET         750.0f
#define SPEED_PLAZMA         800.0f
#define SPEED_BULLET        4000.0f
#define SPEED_TANKBULLET    1400.0f
#define SPEED_GAUSS        10000.0f
#define SPEED_ACBULLET      1800.0f
#define SPEED_DISK          2000.0f
#define SPEED_BFGCORE        500.0f
#define SPEED_FIRE           600.0f
#define SPEED_SMOKE          vec2d(0, -40.0f)


#define DAMAGE_ROCKET_AK47   50.0f
#define DAMAGE_BULLET         4.0f
#define DAMAGE_PLAZMA        30.0f
#define DAMAGE_TANKBULLET    60.0f
#define DAMAGE_BFGCORE       60.0f
#define DAMAGE_FIRE           4.2f
#define DAMAGE_FIRE_HIT       1.2f
#define DAMAGE_ACBULLET      15.0f
#define DAMAGE_GAUSS         60.0f
#define DAMAGE_GAUSS_FADE    15.0f
#define DAMAGE_DISK_MIN      20.0f
#define DAMAGE_DISK_MAX      28.0f
#define DAMAGE_DISK_FADE      3.0f
#define DAMAGE_RAM_ENGINE   100.0f


// default animation speed
#define ANIMATION_FPS        25.0f  // fps


// distance to the crosshair
#define CH_DISTANCE_NORMAL  200.0f  // normal
#define CH_DISTANCE_THIN    150.0f  // for minigun

// tower control
#define TOWER_ROT_SPEED      5.5f   // maximum rotation speed
#define TOWER_ROT_ACCEL     15.5f   // acceleration
#define TOWER_ROT_SLOWDOWN  30.1f   // braking

// operation time
#define BOOSTER_TIME        20.0f
#define PROTECT_TIME        20.0f


//-----------------------------------------------------------------------------
// ai priorities settings

// if the priority <= AIP_NOTREQUIRED than ai ignores the item
#define AIP_NOTREQUIRED     0.0f

// priority unit
#define AIP_NORMAL          1.0f

// the priority rate falls with the distance: p = (base - AIP_NORMAL * l / AI_MAX_DEPTH)
// where base - basic priority level, l - distance in cells


#define AIP_WEAPON_NORMAL   (AIP_NORMAL)        // weapon priority when no weapon at all
#define AIP_WEAPON_FAVORITE (AIP_NORMAL / 2)    // the priority of a favorite weapon
#define AIP_WEAPON_ADVANCED (AIP_NORMAL / 2)    // the priority of a weapon with the booster attached
#define AIP_HEALTH          (AIP_NORMAL)        // the priority of the 'health' item when the ai is almost dead
#define AIP_BOOSTER         (AIP_NORMAL)        // the priority of a booster
#define AIP_BOOSTER_HAVE    (AIP_BOOSTER / 10)  // the priority of a booster if we already have a booster attached
#define AIP_SHOCK           (AIP_NORMAL)        // the priority of a shock item
#define AIP_SHIELD          (AIP_NORMAL)        // the priority of a shield item

#define AI_MAX_DEPTH   50.0f
#define AI_MAX_SIGHT   20.0f
#define AI_MAX_LEVEL   4U

//-----------------------------------------------------------------------------
#define TURET_ROCKET_RELOAD  0.9f
#define TURET_CANON_RELOAD   0.4f
#define TURET_SIGHT_RADIUS   500

//-----------------------------------------------------------------------------
#define PLAYER_RESPAWN_DELAY 2.0f


//-----------------------------------------------------------------------------
// compatibility

#define TXT_VERSION      "Tank: Zone of Death (1.52.4+) Unofficial release"
#define TXT_WNDCLASS     "TankMainWindow"

// this value is used to check files compatibility
#define VERSION    0x1520

//-----------------------------------------------------------------------------

#define MAX_PLAYERS     32
#define MAX_HUMANS       4

#define MAX_TEAMS        6 // including 0 (no team)

#define MAX_DT           0.05f
#define MAX_DT_FIXED     0.02f

//-----------------------------------------------------------------------------
// Game types

// editor mode
// the timelimit and fraglimit settings are not functional in this mode
#define GT_EDITOR        0

#define GT_DEATHMATCH    1

// main menu intro
#define GT_INTRO         2

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
