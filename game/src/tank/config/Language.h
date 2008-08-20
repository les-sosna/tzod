// Language.h


#ifndef CONFIG_CACHE_PASS
 #define CONFIG_CACHE_PASS 1
#endif

#include "ConfigCache.h"



///////////////////////////////////////////////////////////////////////////////
// language map

CONFIG_BEGIN(LangCache) //  var_name  def_value

	CONFIG_VAR_STR( c_locale, "English" )

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

	CONFIG_VAR_STR( get_file_name_title, "File name" )
	CONFIG_VAR_STR( get_file_name_save_game, "Save Game" )
	CONFIG_VAR_STR( get_file_name_load_game, "Load Game" )
	CONFIG_VAR_STR( get_file_name_save_map, "Save map" )
	CONFIG_VAR_STR( get_file_name_load_map, "Select map for edit" )



	//
	// Editor
	//

	CONFIG_VAR_STR( obj_crate, "Object:\tCrate" )
	CONFIG_VAR_STR( obj_wood, "Landscape:\tWood" )
	CONFIG_VAR_STR( obj_respawn, "Respawn point" )
	CONFIG_VAR_STR( obj_spotlight, "Object: Spot light" )
	CONFIG_VAR_STR( obj_health, "Item: Health" )
	CONFIG_VAR_STR( obj_mine, "Item: Anti-tank mine" )
	CONFIG_VAR_STR( obj_shield, "Item: Shield" )
	CONFIG_VAR_STR( obj_shock, "Item: Shock" )
	CONFIG_VAR_STR( obj_booster, "Item: Booster" )
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
	CONFIG_VAR_STR( obj_tank, "Object: Tank" )
	CONFIG_VAR_STR( obj_weap_rockets, "Weapon: Rocket launcher" )
	CONFIG_VAR_STR( obj_weap_autocannon, "Weapon: Auto cannon" )
	CONFIG_VAR_STR( obj_weap_cannon, "Weapon: Cannon" )
	CONFIG_VAR_STR( obj_weap_plazma, "Weapon: Plasma" )
	CONFIG_VAR_STR( obj_weap_gauss, "Weapon: Gauss" )
	CONFIG_VAR_STR( obj_weap_ram, "Weapon: Ram" )
	CONFIG_VAR_STR( obj_weap_bfg, "Weapon: BFG" )
	CONFIG_VAR_STR( obj_weap_ripper, "Weapon: Ripper" )
	CONFIG_VAR_STR( obj_weap_minigun, "Weapon: Minigun" )
	CONFIG_VAR_STR( obj_weap_zippo, "Weapon: Flamethrower" )
	CONFIG_VAR_STR( obj_service_player_ai, "Service: AI player" )
	CONFIG_VAR_STR( obj_service_player_local, "Service: Local player" )


	CONFIG_VAR_STR( service_create, "Create" )
	CONFIG_VAR_STR( service_type, "Service" )
	CONFIG_VAR_STR( service_name, "Name" )



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

	CONFIG_VAR_STR( player_settings, "Player settings" )
	CONFIG_VAR_STR( player_nick, "Nickname" )
	CONFIG_VAR_STR( player_skin, "Skin" )
	CONFIG_VAR_STR( player_team, "Team" )
	CONFIG_VAR_STR( player_class, "Class" )
	CONFIG_VAR_STR( player_profile, "Profile" )

	CONFIG_VAR_STR( bot_settings, "Bot settings" )
	CONFIG_VAR_STR( bot_level, "Experience" )
	CONFIG_VAR_STR( bot_level_0, "Looser" )
	CONFIG_VAR_STR( bot_level_1, "Normal" )
	CONFIG_VAR_STR( bot_level_2, "Advanced" )
	CONFIG_VAR_STR( bot_level_3, "Hardcore" )
	CONFIG_VAR_STR( bot_level_4, "Nightmare" )

	CONFIG_VAR_STR( common_ok, "OK" )
	CONFIG_VAR_STR( common_cancel, "Cancel" )


	//
	// network
	//

	CONFIG_VAR_STR( net_server_title, "Host a new server" )
	CONFIG_VAR_STR( net_server_fps, "Server fps" )
	CONFIG_VAR_STR( net_server_latency, "Latency" )
	CONFIG_VAR_STR( net_server_ok, "Create" )
	CONFIG_VAR_STR( net_server_cancel, "Cancel" )

	CONFIG_VAR_STR( net_connect_title, "Connect" )
	CONFIG_VAR_STR( net_connect_address, "Server address" )
	CONFIG_VAR_STR( net_connect_status, "Status log" )
	CONFIG_VAR_STR( net_connect_ok, "Connect" )
	CONFIG_VAR_STR( net_connect_cancel, "Cancel" )
	CONFIG_VAR_STR( net_connect_error, "Network error" )
	CONFIG_VAR_STR( net_connect_error_server_version, "Incompatible server version" )
	CONFIG_VAR_STR( net_connect_error_map, "Could not load map" )
	CONFIG_VAR_STR( net_connect_error_map_version, "Incompatible map version" )
	CONFIG_VAR_STR( net_connect_loading_map_x, "Loading map '%s'..." )



	//
	// in game messages
	//

	CONFIG_VAR_STR( no_respawns_for_team_x, "There are no respawn points for team %s!" )

CONFIG_END(LangCache, g_lang)


// end of file
