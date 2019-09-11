#include <config/ConfigBase.h>

// first pass to define the structure
#include "CombinedConfig.h"

// second pass to implement initialize function
#define CONFIG_CACHE_PASS2
#include "CombinedConfig.h"
#undef CONFIG_CACHE_PASS2
