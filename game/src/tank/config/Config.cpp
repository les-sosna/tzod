// Config.cpp

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////

// first time include it to define the structure
#include "Config.h"

ConfCache  g_conf;
Config*    g_config = NULL;


///////////////////////////////////////////////////////////////////////////////

#undef CONFIG_BEGIN
#undef CONFIG_VAR_FLOAT
#undef CONFIG_VAR_INT
#undef CONFIG_VAR_BOOL
#undef CONFIG_VAR_STRING
#undef CONFIG_VAR_ARRAY
#undef CONFIG_VAR_CONFIG
#undef CONFIG_END

#define CONFIG_CPP

#define CONFIG_BEGIN()                       \
	void ConfCache::Initialize(Config *cfg)  \


#define CONFIG_VAR_FLOAT( var, def )         \
	this->var = cfg->GetNum( #var, (float) (def) );

#define CONFIG_VAR_INT( var, def )           \
	this->var = cfg->GetNum( #var, (int) (def) );

#define CONFIG_VAR_BOOL( var, def )          \
	this->var = cfg->GetBool( #var, (def) );

#define CONFIG_VAR_STRING( var, def )        \
	this->var = cfg->GetStr( #var, (def) );

#define CONFIG_VAR_ARRAY( var )              \
	this->var = cfg->GetArray( #var );

#define CONFIG_VAR_CONFIG( var )             \
	this->var = cfg->GetConf( #var );

#define CONFIG_END()


///////////////////////////////////////////////////////////////////////////////

// second time include it to implement initialize function
#include "Config.h"

///////////////////////////////////////////////////////////////////////////////
// end of file
