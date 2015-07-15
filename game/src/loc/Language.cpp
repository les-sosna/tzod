// first time include it to define the structure
#define CONFIG_CACHE_PASS 1
#include "inc/loc/Language.h"


// second time include it to implement initialize function
#define CONFIG_CACHE_PASS 2
#include "inc/loc/Language.h"

// global
LangCache g_lang;
