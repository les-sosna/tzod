// Language.h


#ifndef CONFIG_CACHE_PASS
 #define CONFIG_CACHE_PASS 1
#endif

#include "ConfigCache.h"



///////////////////////////////////////////////////////////////////////////////
// language map

CONFIG_BEGIN(LangCache) //  var_name  def_value

	//
	// Main menu
	//

	CONFIG_VAR_STR( single_player_btn, "Single" )
	CONFIG_VAR_STR( single_player_title, "Single player" )
	CONFIG_VAR_STR( single_player_campaign, "Campaign" )
	CONFIG_VAR_STR( single_player_skirmish, "Skirmish (F2)" )
	CONFIG_VAR_STR( single_player_load, "Load game" )
	CONFIG_VAR_STR( single_player_save, "Save game" )

	CONFIG_VAR_STR( network_btn, "Network" )
	CONFIG_VAR_STR( network_title, "Network" )
	CONFIG_VAR_STR( network_host, "Host" )
	CONFIG_VAR_STR( network_join, "Join" )
	CONFIG_VAR_STR( network_profile, "Profile" )

	CONFIG_VAR_STR( editor_btn, "Editor" )
	CONFIG_VAR_STR( editor_title, "Map editor" )
	CONFIG_VAR_STR( editor_new_map, "New map" )
	CONFIG_VAR_STR( editor_load_map, "Load map" )
	CONFIG_VAR_STR( editor_save_map, "Save map" )
	CONFIG_VAR_STR( editor_map_settings, "Map settings" )

	CONFIG_VAR_STR( settings_btn, "Settings (F12)" )

	CONFIG_VAR_STR( exit_game_btn, "Exit (Alt+F4)" )




	//
	// Editor
	//

	CONFIG_VAR_STR( obj_crate, "Object: Crate" )
	CONFIG_VAR_STR( obj_wood, "Landscape: Wood" )
	CONFIG_VAR_STR( obj_respawn, "Respawn point" )
	CONFIG_VAR_STR( obj_spotlight, "Object: Spot light" )
	CONFIG_VAR_STR( obj_health, "Item: Health" )
	CONFIG_VAR_STR( obj_mine, "Item: Mine" )
	CONFIG_VAR_STR( obj_shield, "Item: Shield" )
	CONFIG_VAR_STR( obj_shock, "Item: Shock" )
	CONFIG_VAR_STR( obj_booster, "Item: Booster" )
	CONFIG_VAR_STR( obj_player_local, "Service: Local player" )
	CONFIG_VAR_STR( obj_wall_brick, "Wall: Brick" )
	CONFIG_VAR_STR( obj_wall_concrete, "Wall: Concrete" )
	CONFIG_VAR_STR( obj_water, "Landscape: Water" )
	CONFIG_VAR_STR( obj_trigger, "Trigger" )
	CONFIG_VAR_STR( obj_turret_rocket, "Turret: Rockets" )
	CONFIG_VAR_STR( obj_turret_cannon, "Turret: Cannon" )
	CONFIG_VAR_STR( obj_turret_minigun, "Turret: Minigun" )
	CONFIG_VAR_STR( obj_turret_gauss, "Turret: Gauss gun" )
	CONFIG_VAR_STR( obj_user_object, "User object" )
	CONFIG_VAR_STR( obj_user_sprite, "User sprite" )



	//
	// new dm dialog
	//

	CONFIG_VAR_STR( choose_map, "Choose map" )
	CONFIG_VAR_STR( night_mode, "Night mode" )
	CONFIG_VAR_STR( time_limit, "Time limit" )
	CONFIG_VAR_STR( frag_limit, "Frag limit" )
	CONFIG_VAR_STR( zero_no_limits, "(0 - no limits)" )
	CONFIG_VAR_STR( game_speed, "Game speed, %" )
	CONFIG_VAR_STR( team_none, "[none]" )

	CONFIG_VAR_STR( human_player_list, "Human players" )
	CONFIG_VAR_STR( human_player_add, "Add (Insert)" )
	CONFIG_VAR_STR( human_player_remove, "Remove" )
	CONFIG_VAR_STR( human_player_modify, "Modify" )

	CONFIG_VAR_STR( AI_player_list, "Bots" )
	CONFIG_VAR_STR( AI_player_add, "Add" )
	CONFIG_VAR_STR( AI_player_remove, "Remove" )
	CONFIG_VAR_STR( AI_player_modify, "Modify" )

	CONFIG_VAR_STR( dm_ok, "Go!" )
	CONFIG_VAR_STR( dm_cancel, "Cancel" )



	//
	// in game messages
	//

	CONFIG_VAR_STR( no_respawns_for_team_x, "There are no respawn points for team %s!" )

CONFIG_END(LangCache, g_lang)


// end of file
