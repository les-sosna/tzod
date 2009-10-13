// Config.h
// this file is designed to be included twice
// don't use pragma once
///////////////////////////////////////////////////////////////////////////////

#ifndef CONFIG_CACHE_PASS
 #define CONFIG_CACHE_PASS 1
 #pragma once
#endif

#include "ConfigCache.h"



///////////////////////////////////////////////////////////////////////////////
// config map

REFLECTION_BEGIN(ConfCache) //  var_name  def_value

	// display settings
	VAR_INT(  r_render,           0 )  HELPSTRING("0 - opengl, 1 - d3d")
	VAR_INT(  r_width,         1024 )
	VAR_INT(  r_height,         768 )
	VAR_INT(  r_freq,             0 )
	VAR_INT(  r_bpp,             32 )
	VAR_BOOL( r_fullscreen,    true )
	VAR_BOOL( r_askformode,    true )
	VAR_INT(  r_screenshot,       1 )

	// server settings
	VAR_STR(    sv_name,   "ZOD server" )
	VAR_INT(    sv_port,           1945 )
	VAR_FLOAT(  sv_fps,              30 )
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
	VAR_FLOAT(  cl_boost,             1 )
	VAR_FLOAT(  cl_dtwindow,          2 )
	VAR_TABLE(  cl_playerinfo,     NULL )

	// sound
	VAR_INT( s_volume,      DSBVOLUME_MAX )
	VAR_INT( s_musicvolume, DSBVOLUME_MAX )
	VAR_INT( s_maxchanels,             16 )
	VAR_INT( s_buffer,               1000 )

	// game
	VAR_BOOL(  g_shownames,         true )
	VAR_BOOL(  g_showdamage,       false )
	VAR_BOOL(  g_particles,         true )
	VAR_BOOL(  g_rotcamera,        false )
	VAR_FLOAT( g_rotcamera_a,      10.0f )
	VAR_FLOAT( g_rotcamera_s,      10.0f )
	VAR_FLOAT( g_rotcamera_m,       5.0f )

	// editor
	VAR_BOOL(  ed_drawgrid,         true )
	VAR_BOOL(  ed_uselayers,       false )
	VAR_INT(   ed_width,              32 )
	VAR_INT(   ed_height,             24 )
	VAR_INT(   ed_object,              0 )
	VAR_BOOL(  ed_showproperties,   true )
	VAR_BOOL(  ed_showservices,    false )
	VAR_TABLE( ed_objproperties,    NULL )

	// console
	VAR_INT(   con_maxhistory,        30 )
	VAR_ARRAY( con_history,         NULL )

	// user interface
	VAR_BOOL(  ui_showfps,         false )
	VAR_BOOL(  ui_showtime,         true )
	VAR_BOOL(  ui_showmsg,          true )
	VAR_TABLE( ui_netbotinfo,       NULL )

	// debug
	VAR_BOOL(  dbg_graph,          false )
	VAR_INT(   dbg_sleep,              0 )
	VAR_INT(   dbg_sleep_rand,         0 )

	// other
	VAR_TABLE( dm_profiles,       InitProfiles )
	VAR_ARRAY( dm_players,                NULL )
	VAR_ARRAY( dm_bots,                   NULL )
	VAR_ARRAY( lobby_servers,    InitLobbyList )

REFLECTION_END()

extern ConfCache g_conf;

///////////////////////////////////////////////////////////////////////////////
// end of file
