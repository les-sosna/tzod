#include <config/ConfigBase.h>

// first pass to define the structure
#include "inc/as/AppConfig.h"

// second pass to implement initialize function
#define CONFIG_CACHE_PASS2
#include "inc/as/AppConfig.h"
#undef CONFIG_CACHE_PASS2
