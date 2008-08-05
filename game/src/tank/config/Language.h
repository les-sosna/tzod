// Language.h


#ifndef CONFIG_CACHE_PASS
 #define CONFIG_CACHE_PASS 1
#endif

#include "ConfigCache.h"



///////////////////////////////////////////////////////////////////////////////
// language map

CONFIG_BEGIN(LangCache) //  var_name  def_value

	CONFIG_VAR_STR( choose_map, "Choose map" );
	CONFIG_VAR_STR( night_mode, "Night mode" );
	CONFIG_VAR_STR( time_limit, "Time limit" );
	CONFIG_VAR_STR( frag_limit, "Frag limit" );
	CONFIG_VAR_STR( zero_no_limits, "(0 - no limits)" );
	CONFIG_VAR_STR( game_speed, "Game speed, %" );
	CONFIG_VAR_STR( team_none, "[none]" );

	CONFIG_VAR_STR( no_respawns_for_team_x, "There are no respawn points for team %s!" );

CONFIG_END(LangCache, g_lang)


// end of file
