#include <config/ConfigBase.h>

// first time include it to define the structure
#define CONFIG_CACHE_PASS 1
#include "Config.h"

static void InitProfiles(ConfVarTable &profiles)
{
	ConfControllerProfile(&profiles.GetTable("WASD"));

	ConfControllerProfile arrows(&profiles.GetTable("Arrows"));
	arrows.key_forward.Set("Up");
	arrows.key_back.Set("Down");
	arrows.key_left.Set("Left");
	arrows.key_right.Set("Right");
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

static void InitLobbyList(ConfVarArray &lobby_servers)
{
	lobby_servers.PushBack(ConfVar::typeString).AsStr().Set("tzod.fatal.ru/lobby");
}

// second time include it to implement initialize function
#define CONFIG_CACHE_PASS 2
#include "Config.h"

ConfCache g_conf;
