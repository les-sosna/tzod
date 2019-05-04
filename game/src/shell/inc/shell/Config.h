// This file is designed to be included twice
// Do not use pragma once

#if defined(CONFIG_CACHE_PASS2) && !defined(SHELL_CONFIG_PASS2_INCLUDED) || \
   !defined(CONFIG_CACHE_PASS2) && !defined(SHELL_CONFIG_PASS1_INCLUDED)
#ifdef CONFIG_CACHE_PASS2
# define SHELL_CONFIG_PASS2_INCLUDED
#else
# define SHELL_CONFIG_PASS1_INCLUDED
#endif

#include <config/ConfigCache.h>

#ifndef CONFIG_CACHE_PASS2
# include <ctx/AppConfig.h>
# include <editor/Config.h>
#endif

REFLECTION_BEGIN(ConfControllerProfile)
	VAR_STR(key_forward,      "W")
	VAR_STR(key_back,         "S")
	VAR_STR(key_left,         "A")
	VAR_STR(key_right,        "D")
	VAR_STR(key_fire,     "Space")
	VAR_STR(key_light,        "F")
	VAR_STR(key_tower_left,   "1")
	VAR_STR(key_tower_center, "2")
	VAR_STR(key_tower_right,  "3")
	VAR_STR(key_pickup,       "E")
	VAR_INT_RANGE(gamepad,      0, 0, 4)
	VAR_BOOL(lights,         true)
	VAR_BOOL(aim_to_mouse,   true)
	VAR_BOOL(move_to_mouse, false)
	VAR_BOOL(arcade_style,   true)
REFLECTION_END()

///////////////////////////////////////////////////////////////////////////////

REFLECTION_BEGIN(ShellConfig) //  var_name  def_value

	// display settings
	VAR_INT(  r_width,         1024 )
	VAR_INT(  r_height,         768 )
	VAR_INT(  r_freq,             0 )
	VAR_BOOL( r_fullscreen,   false )

	// server settings
	VAR_STR(    sv_name,   "ZOD server" )
	VAR_INT(    sv_port,           1945 )
	VAR_FLOAT(  sv_fps,              60 )
	VAR_FLOAT(  sv_latency,           0 )
	VAR_FLOAT(  sv_sensitivity,       1 )
	VAR_FLOAT(  sv_speed,           100 )  HELPSTRING("percent")
	VAR_FLOAT(  sv_timelimit,         7 )  HELPSTRING("minutes")
	VAR_INT(    sv_fraglimit,        21 )
	VAR_BOOL(   sv_nightmode,     false )
	VAR_STR(    sv_lobby,            "" )
	VAR_BOOL(   sv_use_lobby,     false )

	// client settings
	VAR_STR(    cl_map,           "dm1" )
	VAR_FLOAT(  cl_speed,           100 ) // percent
	VAR_FLOAT(  cl_timelimit,         7 ) // minutes
	VAR_INT(    cl_fraglimit,        21 )
	VAR_BOOL(   cl_nightmode,     false )
	VAR_STR(    cl_server,  "localhost" )
	VAR_FLOAT(  cl_latency,           0 )
	VAR_REFLECTION( cl_playerinfo, ConfPlayerLocal )

	VAR_REFLECTION(editor, EditorConfig)

	// sound
	VAR_BOOL( s_enabled,      true )
	VAR_INT(  s_volume,      10000 )
	VAR_INT(  s_musicvolume, 10000 )

	// game
	VAR_BOOL( g_shownames, true )

	// console
	VAR_INT(   con_maxhistory,        30 )
	VAR_ARRAY( con_history,         nullptr )

	// user interface
	VAR_FLOAT( ui_tile_size,      256.0f )
	VAR_FLOAT( ui_tile_spacing,    16.0f )
	VAR_FLOAT( ui_foldtime,         0.25f )
	VAR_FLOAT( ui_nav_spacing,    100.0f )
	VAR_REFLECTION( ui_netbotinfo,  ConfPlayerAI )

	// debug
	VAR_BOOL( d_graph,          false )
	VAR_BOOL( d_field,          false )
	VAR_BOOL( d_path,           false )
	VAR_BOOL( d_showfps,        false )
	VAR_BOOL( d_artistmode,     false )

	// other
	VAR_STR(   dm_player1,      "Arrows" )
	VAR_STR(   dm_player2,        "WASD" )
	VAR_TABLE( dm_profiles,       InitProfiles ) // ConfControllerProfile
	VAR_ARRAY( dm_players,                nullptr )
	VAR_ARRAY( dm_bots,                   nullptr )
	VAR_ARRAY( lobby_servers,    InitLobbyList )

	VAR_INT(sp_tier, 0)
	VAR_INT(sp_map, 0)

REFLECTION_END()

class DMCampaign;
int GetCurrentTier(const ShellConfig &conf, const DMCampaign &dmCampaign);
int GetCurrentTierMapCount(const ShellConfig &conf, const DMCampaign &dmCampaign);
int GetCurrentMap(const ShellConfig &conf, const DMCampaign &dmCampaign);

#endif
