// This file is designed to be included twice
// Do not use pragma once

#if defined(CONFIG_CACHE_PASS2) && !defined(COMBINED_CONFIG_PASS2_INCLUDED) || \
   !defined(CONFIG_CACHE_PASS2) && !defined(COMBINED_CONFIG_PASS1_INCLUDED)
#ifdef CONFIG_CACHE_PASS2
# define COMBINED_CONFIG_PASS2_INCLUDED
#else
# define COMBINED_CONFIG_PASS1_INCLUDED
#endif

#include <config/ConfigCache.h>

#ifndef CONFIG_CACHE_PASS2
# include <as/AppConfig.h>
# include <shell/Config.h>
#endif

REFLECTION_BEGIN(CombinedConfig)
	VAR_REFLECTION(shell, ShellConfig)
	VAR_REFLECTION(game, AppConfig)
REFLECTION_END()

#endif
