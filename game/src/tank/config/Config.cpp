// Config.cpp

#include "stdafx.h"
#include "ConfigBase.h"

// first time include it to define the structure
#define CONFIG_CACHE_PASS 1
#include "Config.h"

///////////////////////////////////////////////////////////////////////////////
// initializers

static void InitProfiles(ConfVarTable *profiles)
{
	ConfControllerProfile(profiles->GetTable("WASD"));

	ConfControllerProfile arrows(profiles->GetTable("Arrows"));
	arrows.key_forward.Set("Up Arrow");
	arrows.key_back.Set("Down Arrow");
	arrows.key_left.Set("Left Arrow");
	arrows.key_right.Set("Right Arrow");
	arrows.key_fire.Set("Space");
	arrows.key_light.Set("Delete");
	arrows.key_tower_left.Set("8");
	arrows.key_tower_center.Set("9");
	arrows.key_tower_right.Set("0");
	arrows.key_pickup.Set("Right Shift");
	arrows.lights.Set(true);
	arrows.aim_to_mouse.Set(false);
	arrows.move_to_mouse.Set(false);
	arrows.arcade_style.Set(false);
}

static void InitLobbyList(ConfVarArray *lobby_servers)
{
	lobby_servers->PushBack(ConfVar::typeString)->AsStr()->Set("tzod.fatal.ru/lobby");
	lobby_servers->PushBack(ConfVar::typeString)->AsStr()->Set("tankz.ru/lobby");
}


///////////////////////////////////////////////////////////////////////////////

// second time include it with the CONFIG_CPP defined to implement initialize function
#define CONFIG_CACHE_PASS 2
#include "Config.h"

// global
ConfCache g_conf;

///////////////////////////////////////////////////////////////////////////////
// end of file
