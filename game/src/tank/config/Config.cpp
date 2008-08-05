// Config.cpp

#include "stdafx.h"

// first time include it to define the structure
#define CONFIG_CACHE_PASS 1
#include "Config.h"


// second time include it with the CONFIG_CPP defined to implement initialize function
#define CONFIG_CACHE_PASS 2
#include "Config.h"


///////////////////////////////////////////////////////////////////////////////
// helper functions

void CreateDefaultProfiles()
{
	ConfVarTable *p;

	p = g_conf->dm_profiles->GetTable("Player 1");
	p->SetStr( "key_forward",       "Up Arrow"     );
	p->SetStr( "key_left",          "Left Arrow"   );
	p->SetStr( "key_back",          "Down Arrow"   );
	p->SetStr( "key_right",         "Right Arrow"  );
	p->SetStr( "key_fire",          "Right Ctrl"   );
	p->SetStr( "key_light",         "Delete"       );
	p->SetStr( "key_tower_left",    "Right Alt"    );
	p->SetStr( "key_tower_center",  "Right Win"    );
	p->SetStr( "key_tower_right",   "App Menu"     );
	p->SetStr( "key_pickup",        "Right Shift"  );

	p = g_conf->dm_profiles->GetTable("Player 2");
	p->SetStr( "key_forward",       "W"            );
	p->SetStr( "key_left",          "A"            );
	p->SetStr( "key_back",          "S"            );
	p->SetStr( "key_right",         "D"            );
	p->SetStr( "key_fire",          "Left Ctrl"    );
	p->SetStr( "key_light",         "Q"            );
	p->SetStr( "key_tower_left",    "1"            );
	p->SetStr( "key_tower_center",  "2"            );
	p->SetStr( "key_tower_right",   "3"            );
	p->SetStr( "key_pickup",        "Left Shift"   );

}


///////////////////////////////////////////////////////////////////////////////
// end of file
