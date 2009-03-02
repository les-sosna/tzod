// Config.h
// this file is designed to be included twice
// don't use pragma once
///////////////////////////////////////////////////////////////////////////////

#ifndef CONFIG_CACHE_PASS
 #define CONFIG_CACHE_PASS 1
#endif

#include "ConfigCache.h"



///////////////////////////////////////////////////////////////////////////////
// config map

CONFIG_BEGIN(ConfCache) //  var_name  def_value

	// display settings
	CONFIG_VAR_INT(  r_render,           0 )  // 0 - opengl, 1 - d3d
	CONFIG_VAR_INT(  r_width,         1024 )
	CONFIG_VAR_INT(  r_height,         768 )
	CONFIG_VAR_INT(  r_freq,             0 )
	CONFIG_VAR_INT(  r_bpp,             32 )
	CONFIG_VAR_BOOL( r_fullscreen,    true )
	CONFIG_VAR_BOOL( r_askformode,    true )
	CONFIG_VAR_INT(  r_screenshot,       1 )

	// server settings
	CONFIG_VAR_STR(    sv_name,   "ZOD server" )
	CONFIG_VAR_INT(    sv_port,           1945 )
	CONFIG_VAR_FLOAT(  sv_fps,              30 )
	CONFIG_VAR_FLOAT(  sv_latency,           1 )
	CONFIG_VAR_BOOL(   sv_use_lobby,     false )
	CONFIG_VAR_BOOL(   sv_autoLatency,    true )
	CONFIG_VAR_FLOAT(  sv_speed,           100 ) // percent
	CONFIG_VAR_FLOAT(  sv_timelimit,         7 ) // minutes
	CONFIG_VAR_INT(    sv_fraglimit,        21 )
	CONFIG_VAR_BOOL(   sv_nightmode,     false )

	// client settings
	CONFIG_VAR_STR(    cl_map,           "dm1" )
	CONFIG_VAR_FLOAT(  cl_speed,           100 ) // percent
	CONFIG_VAR_FLOAT(  cl_timelimit,         7 ) // minutes
	CONFIG_VAR_INT(    cl_fraglimit,        21 )
	CONFIG_VAR_BOOL(   cl_nightmode,     false )
	CONFIG_VAR_STR(    cl_server,  "localhost" )
	CONFIG_VAR_FLOAT(  cl_latency,           4 )
	CONFIG_VAR_TABLE(  cl_playerinfo )

	// sound
	CONFIG_VAR_INT( s_volume,      DSBVOLUME_MAX )
	CONFIG_VAR_INT( s_musicvolume, DSBVOLUME_MAX )
	CONFIG_VAR_INT( s_maxchanels,             16 )
	CONFIG_VAR_INT( s_buffer,               1000 )

	// game
	CONFIG_VAR_BOOL(  g_showdamage,       false )
	CONFIG_VAR_BOOL(  g_particles,         true )
	CONFIG_VAR_BOOL(  g_rotcamera,        false )
	CONFIG_VAR_FLOAT( g_rotcamera_a,      10.0f )
	CONFIG_VAR_FLOAT( g_rotcamera_s,      10.0f )
	CONFIG_VAR_FLOAT( g_rotcamera_m,       5.0f )

	// editor
	CONFIG_VAR_BOOL(  ed_drawgrid,         true )
	CONFIG_VAR_BOOL(  ed_uselayers,       false )
	CONFIG_VAR_INT(   ed_width,              32 )
	CONFIG_VAR_INT(   ed_height,             24 )
	CONFIG_VAR_INT(   ed_object,              0 )
	CONFIG_VAR_BOOL(  ed_showproperties,   true )
	CONFIG_VAR_BOOL(  ed_showservices,    false )
	CONFIG_VAR_TABLE( ed_objproperties )

	// console
	CONFIG_VAR_INT(   con_maxhistory,        30 )
	CONFIG_VAR_ARRAY( con_history )

	// user interface
	CONFIG_VAR_BOOL(  ui_showfps,         false )
	CONFIG_VAR_BOOL(  ui_showtime,         true )
	CONFIG_VAR_BOOL(  ui_showmsg,          true )
	CONFIG_VAR_TABLE( ui_netbotinfo )

	// other
	CONFIG_VAR_TABLE( dm_profiles )
	CONFIG_VAR_ARRAY( dm_players  )
	CONFIG_VAR_ARRAY( dm_bots     )
	CONFIG_VAR_ARRAY( lobby_servers )

CONFIG_END(ConfCache, g_conf)


///////////////////////////////////////////////////////////////////////////////
// helper functions

void CreateDefaultProfiles();



///////////////////////////////////////////////////////////////////////////////
// end of file
