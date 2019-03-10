// This file is designed to be included twice
// Do not use pragma once

#if defined(CONFIG_CACHE_PASS2) && !defined(EDITOR_CONFIG_PASS2_INCLUDED) || \
   !defined(CONFIG_CACHE_PASS2) && !defined(EDITOR_CONFIG_PASS1_INCLUDED)
#ifdef CONFIG_CACHE_PASS2
# define EDITOR_CONFIG_PASS2_INCLUDED
#else
# define EDITOR_CONFIG_PASS1_INCLUDED
#endif

#include <config/ConfigCache.h>

REFLECTION_BEGIN(EditorConfig)
	VAR_BOOL(  drawgrid,         true )
	VAR_BOOL(  uselayers,       false )
	VAR_INT(   width,            1024 ) // 1M blocks "should be enough for everyone"
	VAR_INT(   height,           1024 )
	VAR_INT(   object,              0 )
	VAR_BOOL(  showservices,    false )
	VAR_TABLE( objproperties, nullptr )
REFLECTION_END()

#endif
