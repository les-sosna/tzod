// Config.h
// this file is designed to be included twice
// don't use pragma once
///////////////////////////////////////////////////////////////////////////////

#include "ConfigBase.h"


#ifndef CONFIG_CPP

#define CONFIG_BEGIN() struct ConfCache { void Initialize(ConfVarTable *cfg);
#define CONFIG_END()  };

#define CONFIG_VAR_FLOAT( var, def )  ConfVarNumber *var;
#define CONFIG_VAR_INT(   var, def )  ConfVarNumber *var;
#define CONFIG_VAR_BOOL(  var, def )  ConfVarBool   *var;
#define CONFIG_VAR_STR(   var, def )  ConfVarString *var;
#define CONFIG_VAR_ARRAY( var )       ConfVarArray  *var;
#define CONFIG_VAR_TABLE( var )       ConfVarTable  *var;

#endif


///////////////////////////////////////////////////////////////////////////////
// config map

CONFIG_BEGIN()    //  var_name      def_value

	// display settings
	CONFIG_VAR_INT(  r_render,           0 ); // 0 - opengl, 1 - d3d
	CONFIG_VAR_INT(  r_width,         1024 );
	CONFIG_VAR_INT(  r_height,         768 );
	CONFIG_VAR_INT(  r_freq,             0 );
	CONFIG_VAR_INT(  r_bpp,             32 );
	CONFIG_VAR_BOOL( r_fullscreen,    true );
	CONFIG_VAR_BOOL( r_askformode,    true );
	CONFIG_VAR_INT(  r_screenshot,       1 );


	// server settings
	CONFIG_VAR_STR(    sv_name,   "ZOD server" );
	CONFIG_VAR_FLOAT(  sv_fps,              30 );
	CONFIG_VAR_FLOAT(  sv_speed,           100 ); // percent
	CONFIG_VAR_FLOAT(  sv_timelimit,         7 ); // minutes
	CONFIG_VAR_INT(    sv_fraglimit,        21 );
	CONFIG_VAR_BOOL(   sv_nightmode,     false );

	// client settings
	CONFIG_VAR_STR(    cl_map,           "dm1" );
	CONFIG_VAR_INT(    cl_latency,           1 );
	CONFIG_VAR_FLOAT(  cl_speed,           100 ); // percent
	CONFIG_VAR_FLOAT(  cl_timelimit,         7 ); // minutes
	CONFIG_VAR_INT(    cl_fraglimit,        21 );
	CONFIG_VAR_BOOL(   cl_nightmode,     false );
	CONFIG_VAR_STR(    cl_server,  "localhost" );

	// sound
	CONFIG_VAR_INT( s_volume,     DSBVOLUME_MAX );
	CONFIG_VAR_INT( s_maxchanels,            16 );

	// game
	CONFIG_VAR_BOOL(  g_showdamage,       false );
	CONFIG_VAR_BOOL(  g_particles,         true );
	CONFIG_VAR_BOOL(  g_rotcamera,        false );
	CONFIG_VAR_FLOAT( g_rotcamera_a,       2.0f );
	CONFIG_VAR_FLOAT( g_rotcamera_s,       0.5f );
	CONFIG_VAR_FLOAT( g_rotcamera_m,       2.0f );

	// editor
	CONFIG_VAR_BOOL( ed_drawgrid,          true );
	CONFIG_VAR_BOOL( ed_uselayers,        false );
	CONFIG_VAR_INT(  ed_width,               32 );
	CONFIG_VAR_INT(  ed_height,              24 );
	CONFIG_VAR_INT(  ed_object,               0 );

	// console
	CONFIG_VAR_INT(   con_maxhistory,        30 );
	CONFIG_VAR_ARRAY( con_history );

	// user interface
	CONFIG_VAR_BOOL( ui_showfps,          false );
	CONFIG_VAR_BOOL( ui_showtime,          true );

	// other
	CONFIG_VAR_TABLE( dm_profiles );
	CONFIG_VAR_ARRAY( dm_players  );
	CONFIG_VAR_ARRAY( dm_bots     );

CONFIG_END()

///////////////////////////////////////////////////////////////////////////////

extern ConfCache       g_conf;
extern ConfVarTable*   g_config;


///////////////////////////////////////////////////////////////////////////////
// helper functions

void CreateDefaultProfiles();



///////////////////////////////////////////////////////////////////////////////
// end of file
