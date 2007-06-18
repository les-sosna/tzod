// Config.cpp

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////

// first time include it to define the structure
#include "Config.h"

ConfCache      g_conf;
ConfVarTable*  g_config = NULL;


///////////////////////////////////////////////////////////////////////////////

#undef CONFIG_BEGIN
#undef CONFIG_VAR_FLOAT
#undef CONFIG_VAR_INT
#undef CONFIG_VAR_BOOL
#undef CONFIG_VAR_STR
#undef CONFIG_VAR_ARRAY
#undef CONFIG_VAR_TABLE
#undef CONFIG_END

#define CONFIG_CPP


#define CONFIG_BEGIN() void ConfCache::Initialize(ConfVarTable *cfg) {
#define CONFIG_END() }

#define CONFIG_VAR_FLOAT( var, def )  this->var = cfg->GetNum(   #var, (float) (def) );
#define CONFIG_VAR_INT(   var, def )  this->var = cfg->GetNum(   #var, (int) (def) );
#define CONFIG_VAR_BOOL(  var, def )  this->var = cfg->GetBool(  #var, (def) );
#define CONFIG_VAR_STR(   var, def )  this->var = cfg->GetStr(   #var, (def) );
#define CONFIG_VAR_ARRAY( var )       this->var = cfg->GetArray( #var );
#define CONFIG_VAR_TABLE( var )       this->var = cfg->GetTable( #var );

///////////////////////////////////////////////////////////////////////////////

// second time include it to implement initialize function
#include "Config.h"

///////////////////////////////////////////////////////////////////////////////
// helper functions

void CreateDefaultProfiles()
{
	ConfVarTable *p;

	p = g_conf.dm_profiles->GetTable("Player 1");
	p->SetStr( "key_forward",       "Up Arrow"     );
	p->SetStr( "key_back",          "Down Arrow"   );
	p->SetStr( "key_left",          "Left Arrow"   );
	p->SetStr( "key_right",         "Right Arrow"  );
	p->SetStr( "key_fire",          "Right Ctrl"   );
	p->SetStr( "key_light",         "Delete"       );
	p->SetStr( "key_tower_left",    "Right Alt"    );
	p->SetStr( "key_tower_right",   "App Menu"     );
	p->SetStr( "key_tower_center",  "Right Win"    );
	p->SetStr( "key_pickup",        "Right Shift"  );

		
}


///////////////////////////////////////////////////////////////////////////////
// end of file
