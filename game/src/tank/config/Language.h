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
	CONFIG_VAR_STR( network_internet, "Internet" )
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

	CONFIG_VAR_STR( campaign_title, "Start a new campaign" )
	CONFIG_VAR_STR( campaign_ok, "Let's go!" )
	CONFIG_VAR_STR( campaign_cancel, "Cancel" )


	//
	// network
	//

	CONFIG_VAR_STR( net_server_title, "Host a new server" )
	CONFIG_VAR_STR( net_server_fps, "Server fps" )
	CONFIG_VAR_STR( net_server_latency, "Latency" )
	CONFIG_VAR_STR( net_server_use_lobby, "Global (use lobby server)" )
	CONFIG_VAR_STR( net_server_add_lobby, "Add" )
	CONFIG_VAR_STR( net_server_ok, "Create" )
	CONFIG_VAR_STR( net_server_cancel, "Cancel" )
	CONFIG_VAR_STR( net_server_error, "Could not start server. Check your firewall settings." )

	CONFIG_VAR_STR( net_internet_title, "Search the Internet" )
	CONFIG_VAR_STR( net_internet_refresh, "Refresh" )
	CONFIG_VAR_STR( net_internet_connect, "Connect" )
	CONFIG_VAR_STR( net_internet_cancel, "Cancel" )
	CONFIG_VAR_STR( net_internet_searching, "Searching..." )
	CONFIG_VAR_STR( net_internet_not_found, "No servers found" )
	CONFIG_VAR_STR( net_internet_server_list, "Server list" )

	CONFIG_VAR_STR( net_connect_title, "Connect" )
	CONFIG_VAR_STR( net_connect_address, "Server address" )
	CONFIG_VAR_STR( net_connect_status, "Status log" )
	CONFIG_VAR_STR( net_connect_ok, "Connect" )
	CONFIG_VAR_STR( net_connect_cancel, "Cancel" )
	CONFIG_VAR_STR( net_connect_error_server_version, "Incompatible server version" )
	CONFIG_VAR_STR( net_connect_error_map, "Could not load map" )
	CONFIG_VAR_STR( net_connect_error_map_version, "Incompatible map version" )
//	CONFIG_VAR_STR( net_connect_loading_map_x, "Loading map '%s'..." )

	CONFIG_VAR_STR( net_chatroom_title, "Waiting for players" )
	CONFIG_VAR_STR( net_chatroom_players, "Players" )
	CONFIG_VAR_STR( net_chatroom_my_profile, "My profile" )
	CONFIG_VAR_STR( net_chatroom_bots, "Bots" )
	CONFIG_VAR_STR( net_chatroom_bot_new, "New bot" )
	CONFIG_VAR_STR( net_chatroom_chat_window, "Chat window" )
	CONFIG_VAR_STR( net_chatroom_ready_button, "I am ready!" )
	CONFIG_VAR_STR( net_chatroom_player_ready, "Ready" )
	CONFIG_VAR_STR( net_chatroom_player_x_disconnected, "%s disconnected" )
	CONFIG_VAR_STR( net_chatroom_player_x_connected, "%s connected" )
	CONFIG_VAR_STR( net_chatroom_team, "team " )

	CONFIG_VAR_STR( net_msg_connecting, "Connecting..." )
	CONFIG_VAR_STR( net_msg_connection_failed, "Connection failed." )
	CONFIG_VAR_STR( net_msg_connection_established, "Connection established" )
	CONFIG_VAR_STR( net_msg_starting_game, "All players ready. Starting game..." )


	//
	// Settings
	//

	CONFIG_VAR_STR( settings_title, "Settings" )
	CONFIG_VAR_STR( settings_profiles, "Profiles" )
	CONFIG_VAR_STR( settings_profile_new, "Create new" )
	CONFIG_VAR_STR( settings_profile_edit, "Edit" )
	CONFIG_VAR_STR( settings_profile_delete, "Delete" )
	CONFIG_VAR_STR( settings_show_fps, "Show FPS" )
	CONFIG_VAR_STR( settings_show_time, "Show time" )
	CONFIG_VAR_STR( settings_show_particles, "Show particles" )
	CONFIG_VAR_STR( settings_show_damage, "Show damage" )
	CONFIG_VAR_STR( settings_ask_for_display_mode, "Ask for display mode at startup" )
	CONFIG_VAR_STR( settings_sfx_volume, "Sfx volume" )
	CONFIG_VAR_STR( settings_music_volume, "Music volume" )

	CONFIG_VAR_STR( profile_name, "Profile name" )
	CONFIG_VAR_STR( profile_autoname, "Profile " )
	CONFIG_VAR_STR( profile_action, "Action" )
	CONFIG_VAR_STR( profile_key, "Bound key" )
	CONFIG_VAR_STR( profile_mouse_aim, "Mouse aim" )
	CONFIG_VAR_STR( profile_mouse_move, "Mouse move" )

	CONFIG_VAR_STR( action_move_forward, "Forward" )
	CONFIG_VAR_STR( action_move_backward, "Backward" )
	CONFIG_VAR_STR( action_turn_left, "Turn left" )
	CONFIG_VAR_STR( action_turn_right, "Turn right" )
	CONFIG_VAR_STR( action_fire, "Fire" )
	CONFIG_VAR_STR( action_toggle_lights, "Toggle lights" )
	CONFIG_VAR_STR( action_tower_left, "Turn weapon left" )
	CONFIG_VAR_STR( action_tower_right, "Turn weapon right" )
	CONFIG_VAR_STR( action_tower_center, "Center weapon" )
	CONFIG_VAR_STR( action_pickup, "Pickup" )


	//
	// Editor
	//

	CONFIG_VAR_STR( map_title, "Map settings" )
	CONFIG_VAR_STR( map_author, "Author" )
	CONFIG_VAR_STR( map_email, "E-Mail" )
	CONFIG_VAR_STR( map_url, "Url" )
	CONFIG_VAR_STR( map_desc, "Description" )
	CONFIG_VAR_STR( map_init_script, "on_init script" )
	CONFIG_VAR_STR( map_theme, "Theme" )

	CONFIG_VAR_STR( newmap_title, "Create a new map" )
	CONFIG_VAR_STR( newmap_width, "Width" )
	CONFIG_VAR_STR( newmap_height, "Height" )

	CONFIG_VAR_STR( layer, "Layer " )
	CONFIG_VAR_STR( f1_help_editor,
		"F1                   - this help\n"
		"F5                   - toggle editor\n"
		"F8                   - map settings\n"
		"F9                   - toggle layers\n"
		"G                    - toggle grid\n"
		"S                    - toggle service list\n"
		"ESC                  - main menu\n"
		"Delete               - delete selected object\n"
		"Enter                - properties\n"
		"Arrows               - move camera\n"
		"Mouse wheel scroll   - choose object type to create\n"
		"Left mouse button    - object create/select/modify\n"
		"Right mouse button   - delete object\n"
		"\nPress and hold Ctrl to create object with default properties" )



	//
	// in game messages
	//

	CONFIG_VAR_STR( score_time_left_xx, "Time left  %d:%02d" )
	CONFIG_VAR_STR( score_frags_left_x, "Frags left %d" )
	CONFIG_VAR_STR( score_time_limit_hit, "Time limit hit" )
	CONFIG_VAR_STR( score_frag_limit_hit, "Frag limit hit" )

	CONFIG_VAR_STR( msg_no_respawns_for_team_x, "There are no respawn points for team %s!" )
	CONFIG_VAR_STR( msg_player_x_killed_him_self, "Stupid %s killed him self" )
	CONFIG_VAR_STR( msg_player_x_killed_his_friend_x, "%s killed his friend %s" )
	CONFIG_VAR_STR( msg_player_x_killed_his_enemy_x, "%s killed his enemy %s" )
	CONFIG_VAR_STR( msg_player_x_was_killed_by_turret, "%s died because of an incident" )
	CONFIG_VAR_STR( msg_player_x_died, "%s died" )
	CONFIG_VAR_STR( msg_server_quit, "Server quit" )
	CONFIG_VAR_STR( msg_player_quit, "Player quit" )

CONFIG_END(LangCache, g_lang)


// end of file
