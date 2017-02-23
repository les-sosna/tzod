// This file is designed to be included twice
// Do not use pragma once

#if defined(CONFIG_CACHE_PASS2) && !defined(LOC_LANGUAGE_PASS2_INCLUDED) || \
   !defined(CONFIG_CACHE_PASS2) && !defined(LOC_LANGUAGE_PASS1_INCLUDED)
#ifdef CONFIG_CACHE_PASS2
# define LOC_LANGUAGE_PASS2_INCLUDED
#else
# define LOC_LANGUAGE_PASS1_INCLUDED
#endif

#include <config/ConfigCache.h>


///////////////////////////////////////////////////////////////////////////////
// language map

REFLECTION_BEGIN(LangCache)

	VAR_STR( c_locale, "English" )

	//
	// Main menu
	//

	VAR_STR( single_player_btn, "1 PLAYER" )

	VAR_STR( editor_btn, "CONSTRUCTION" )
	VAR_STR( editor_new_map, "New map" )
	VAR_STR( editor_load_map, "Load map" )
	VAR_STR( editor_save_map, "Save map" )
	VAR_STR( editor_map_settings, "Map settings" )

	VAR_STR( settings_btn, "SETTINGS" )

	VAR_STR( get_file_name_title, "File name" )
	VAR_STR( get_file_name_save_game, "Save Game" )
	VAR_STR( get_file_name_load_game, "Load Game" )
	VAR_STR( get_file_name_save_map, "Save map" )
	VAR_STR( get_file_name_load_map, "Select map to edit" )
	VAR_STR( get_file_name_new_map, "<Create new map>" )



	//
	// Editor
	//

	VAR_STR( obj_crate, "Object:\tCrate" )
	VAR_STR( obj_wood, "Landscape:\tWood" )
	VAR_STR( obj_respawn, "Respawn point" )
	VAR_STR( obj_spotlight, "Object: Spot light" )
	VAR_STR( obj_health, "Item: Health" )
	VAR_STR( obj_mine, "Item: Anti-tank mine" )
	VAR_STR( obj_shield, "Item: Shield" )
	VAR_STR( obj_shock, "Item: Shock" )
	VAR_STR( obj_booster, "Item: Booster" )
	VAR_STR( obj_wall_brick, "Wall: Brick" )
	VAR_STR( obj_wall_concrete, "Wall: Concrete" )
	VAR_STR( obj_water, "Landscape: Water" )
	VAR_STR( obj_trigger, "Trigger" )
	VAR_STR( obj_turret_rocket, "Turret: Rockets" )
	VAR_STR( obj_turret_cannon, "Turret: Cannon" )
	VAR_STR( obj_turret_minigun, "Turret: Minigun" )
	VAR_STR( obj_turret_gauss, "Turret: Gauss gun" )
	VAR_STR( obj_user_object, "User object" )
	VAR_STR( obj_user_sprite, "User sprite" )
	VAR_STR( obj_tank, "Object: Tank" )
	VAR_STR( obj_weap_rockets, "Weapon: Rocket launcher" )
	VAR_STR( obj_weap_autocannon, "Weapon: Auto cannon" )
	VAR_STR( obj_weap_cannon, "Weapon: Cannon" )
	VAR_STR( obj_weap_plazma, "Weapon: Plasma" )
	VAR_STR( obj_weap_gauss, "Weapon: Gauss" )
	VAR_STR( obj_weap_ram, "Weapon: Ram" )
	VAR_STR( obj_weap_bfg, "Weapon: BFG" )
	VAR_STR( obj_weap_ripper, "Weapon: Ripper" )
	VAR_STR( obj_weap_minigun, "Weapon: Minigun" )
	VAR_STR( obj_weap_zippo, "Weapon: Flamethrower" )
	VAR_STR( obj_service_player, "Service: Player" )
	VAR_STR( obj_service_msgbox, "Service: Message box" )


	VAR_STR( service_create, "Create" )
	VAR_STR( service_type, "Service" )
	VAR_STR( service_name, "Name" )

	VAR_STR( ed_delete, "Delete")



	//
	// new dm dialog
	//

	VAR_STR( choose_map, "Choose map" )
	VAR_STR( night_mode, "Night mode" )
	VAR_STR( time_limit, "Time limit" )
	VAR_STR( frag_limit, "Frag limit" )
	VAR_STR( zero_no_limits, "(0 - no limits)" )
	VAR_STR( game_speed, "Game speed, %" )
	VAR_STR( team_none, "[none]" )

	VAR_STR( human_player_list, "Human players" )
	VAR_STR( human_player_add, "Add Human" )
	VAR_STR( human_player_remove, "Remove" )
	VAR_STR( human_player_modify, "Modify" )

	VAR_STR( AI_player_list, "Bots" )
	VAR_STR( AI_player_add, "Add Bot" )
	VAR_STR( AI_player_remove, "Remove" )
	VAR_STR( AI_player_modify, "Modify" )

	VAR_STR( dm_ok, "Go!" )

	VAR_STR( player_settings, "Player settings" )
	VAR_STR( player_nick, "Nickname" )
	VAR_STR( player_skin, "Skin" )
	VAR_STR( player_team, "Team" )
	VAR_STR( player_class, "Class" )
	VAR_STR( player_profile, "Profile" )

	VAR_STR( bot_settings, "Bot settings" )
	VAR_STR( bot_difficulty, "Difficulty" )
	VAR_STR( bot_difficulty_0, "Easy" )
	VAR_STR( bot_difficulty_1, "Normal" )
	VAR_STR( bot_difficulty_2, "Advanced" )
	VAR_STR( bot_difficulty_3, "Hardcore" )
	VAR_STR( bot_difficulty_4, "Nightmare" )

	VAR_STR( common_ok, "OK" )
	VAR_STR( common_cancel, "Cancel" )

	VAR_STR( campaign_title, "Start a new campaign" )
	VAR_STR( campaign_ok, "Let's go!" )

	VAR_STR( dmcampaign_timelimit, "Time limit: " )
	VAR_STR( dmcampaign_fraglimit, "Frag limit: " )
	VAR_STR( dmcampaign_versus, "VS." )
	VAR_STR( dmcampaign_ok, "PLAY" )


	//
	// network
	//

	VAR_STR( net_server_title, "Host a new server" )
	VAR_STR( net_server_fps, "Server fps" )
	VAR_STR( net_server_latency, "Latency" )
	VAR_STR( net_server_use_lobby, "Global (use lobby server)" )
	VAR_STR( net_server_add_lobby, "Add" )
	VAR_STR( net_server_ok, "Create" )
	VAR_STR( net_server_cancel, "Cancel" )
	VAR_STR( net_server_error, "Could not start server. Check your firewall settings." )

	VAR_STR( net_internet_title, "Search the Internet" )
	VAR_STR( net_internet_lobby_address, "Lobby server address" )
	VAR_STR( net_internet_refresh, "Refresh" )
	VAR_STR( net_internet_connect, "Connect" )
	VAR_STR( net_internet_cancel, "Cancel" )
	VAR_STR( net_internet_searching, "Searching..." )
	VAR_STR( net_internet_not_found, "No servers found" )
	VAR_STR( net_internet_server_list, "Server list" )

	VAR_STR( net_connect_title, "Connect" )
	VAR_STR( net_connect_address, "Server address" )
	VAR_STR( net_connect_status, "Status log" )
	VAR_STR( net_connect_ok, "Connect" )
	VAR_STR( net_connect_cancel, "Cancel" )
	VAR_STR( net_connect_error_server_version, "Incompatible server version" )
	VAR_STR( net_connect_error_map, "Could not load map" )
	VAR_STR( net_connect_error_map_version, "Incompatible map version" )
//	VAR_STR( net_connect_loading_map_x, "Loading map '%s'..." )

	VAR_STR( net_chatroom_title, "Waiting for players" )
	VAR_STR( net_chatroom_players, "Players" )
	VAR_STR( net_chatroom_my_profile, "My profile" )
	VAR_STR( net_chatroom_bots, "Bots" )
	VAR_STR( net_chatroom_bot_new, "New bot" )
	VAR_STR( net_chatroom_chat_window, "Chat window" )
	VAR_STR( net_chatroom_ready_button, "I am ready!" )
	VAR_STR( net_chatroom_player_ready, "Ready" )
	VAR_STR( net_chatroom_player_x_disconnected, "%s disconnected" )
	VAR_STR( net_chatroom_player_x_connected, "%s connected" )
	VAR_STR( net_chatroom_team, "team " )

	VAR_STR( net_msg_connecting, "Connecting..." )
	VAR_STR( net_msg_connection_failed, "Connection failed." )
	VAR_STR( net_msg_connection_established, "Connection established" )
	VAR_STR( net_msg_starting_game, "All players ready. Starting game..." )


	//
	// Settings
	//

	VAR_STR( settings_title, "Settings" )
	VAR_STR( settings_player1, "Player 1" )
	VAR_STR( settings_player2, "Player 2" )
	VAR_STR( settings_profiles, "Profiles" )
	VAR_STR( settings_profile_new, "Create new" )
	VAR_STR( settings_profile_edit, "Edit" )
	VAR_STR( settings_profile_delete, "Delete" )
	VAR_STR( settings_show_fps, "Show FPS" )
	VAR_STR( settings_show_time, "Show time" )
	VAR_STR( settings_show_names, "Show player names" )
	VAR_STR( settings_ask_for_display_mode, "Ask for display mode at startup" )
	VAR_STR( settings_sfx_volume, "Sfx volume" )
	VAR_STR( settings_music_volume, "Music volume" )

	VAR_STR( profile_name, "Profile name" )
	VAR_STR( profile_autoname, "Profile " )
	VAR_STR( profile_action, "Action" )
	VAR_STR( profile_key, "Bound key" )
	VAR_STR( profile_mouse_aim, "Mouse aim" )
	VAR_STR( profile_mouse_move, "Mouse move" )
	VAR_STR( profile_arcade_style, "Arcade style" )

	VAR_STR( action_move_forward, "Forward" )
	VAR_STR( action_move_backward, "Backward" )
	VAR_STR( action_turn_left, "Turn left" )
	VAR_STR( action_turn_right, "Turn right" )
	VAR_STR( action_fire, "Fire" )
	VAR_STR( action_toggle_lights, "Toggle lights" )
	VAR_STR( action_tower_left, "Turn weapon left" )
	VAR_STR( action_tower_right, "Turn weapon right" )
	VAR_STR( action_tower_center, "Center weapon" )
	VAR_STR( action_no_pickup, "Do not pickup" )


	//
	// Editor
	//

	VAR_STR( map_title, "Map settings" )
	VAR_STR( map_author, "Author" )
	VAR_STR( map_email, "E-Mail" )
	VAR_STR( map_url, "Url" )
	VAR_STR( map_desc, "Description" )
	VAR_STR( map_init_script, "on_init script" )
	VAR_STR( map_theme, "Theme" )

	VAR_STR( newmap_title, "Create a new map" )
	VAR_STR( newmap_width, "Width" )
	VAR_STR( newmap_height, "Height" )

	VAR_STR( layer, "Layer " )
	VAR_STR( f1_help_editor,
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

	VAR_STR( score_time_left,  "Time left " )
	VAR_STR( score_frags_left, "Frags left" )
	VAR_STR( score_time_limit_hit, "Time limit hit" )
	VAR_STR( score_frag_limit_hit, "Frag limit hit" )

	VAR_STR( msg_no_respawns_for_team_x, "There are no respawn points for team %s!" )
	VAR_STR( msg_player_x_killed_him_self, "Stupid %s killed him self" )
	VAR_STR( msg_player_x_killed_his_friend_x, "%s killed his friend %s" )
	VAR_STR( msg_player_x_killed_his_enemy_x, "%s killed his enemy %s" )
	VAR_STR( msg_player_x_died, "%s died because of an incident" )
	VAR_STR( msg_server_quit, "Server quit" )
	VAR_STR( msg_player_quit, "Player quit" )

	VAR_STR( msg_no_game_started, "No game started" )

REFLECTION_END()

#endif
