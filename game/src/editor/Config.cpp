#include <config/ConfigBase.h>

// first pass to define the structure
#include "inc/editor/Config.h"

// second time include it to implement initialize function
#define CONFIG_CACHE_PASS2
#include "inc/editor/Config.h"
#undef CONFIG_CACHE_PASS2
