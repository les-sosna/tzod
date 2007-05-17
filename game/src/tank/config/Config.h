// Config.h
// this file is designed to be included twice
// don't use pragma once
///////////////////////////////////////////////////////////////////////////////

#include "ConfigBase.h"


#ifndef CONFIG_CPP

#define CONFIG_BEGIN()                  \
	struct ConfCache                    \
	{                                   \
		void Initialize(Config *cfg);   \
		struct


#define CONFIG_VAR_FLOAT( var, def )    \
	ConfVarNumber *var;

#define CONFIG_VAR_INT( var, def )      \
	ConfVarNumber *var;

#define CONFIG_VAR_STRING( var, def )   \
	ConfVarString *var;


#define CONFIG_END()                    \
	;};

#endif


///////////////////////////////////////////////////////////////////////////////
// config map

CONFIG_BEGIN()    //  var_name      def_value
{
	// display settings
	CONFIG_VAR_INT( r_render,           0 ); // 0 - opengl, 1 - d3d
	CONFIG_VAR_INT( r_width,         1024 );
	CONFIG_VAR_INT( r_height,         768 );
	CONFIG_VAR_INT( r_freq,             0 );
	CONFIG_VAR_INT( r_bpp,             32 );
	CONFIG_VAR_INT( r_fullscreen,       1 );
	CONFIG_VAR_INT( r_askformode,       1 );

	// server settings
	CONFIG_VAR_STRING( sv_name,   "ZOD server" );
	CONFIG_VAR_FLOAT(  sv_fps,              30 );

	// sound
	CONFIG_VAR_INT( s_volume,     DSBVOLUME_MAX );
	CONFIG_VAR_INT( s_maxchanels,            16 );



}
CONFIG_END()

///////////////////////////////////////////////////////////////////////////////

extern ConfCache g_conf;
extern Config*   g_config;

///////////////////////////////////////////////////////////////////////////////
// end of file
