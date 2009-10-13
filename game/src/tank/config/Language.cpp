// Language.cpp

#include "stdafx.h"


// first time include it to define the structure
#define CONFIG_CACHE_PASS 1
#include "Language.h"


// second time include it with the CONFIG_CPP defined to implement initialize function
#define CONFIG_CACHE_PASS 2
#include "Language.h"

// global
LangCache g_lang;

// end of file
