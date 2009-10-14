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
	ConfControllerProfile(profiles->GetTable("Unnamed"));
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
