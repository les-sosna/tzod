// This file is designed to be included twice
// Do not use pragma once

#if defined(CONFIG_CACHE_PASS2) && !defined(APP_CONFIG_PASS2_INCLUDED) || \
   !defined(CONFIG_CACHE_PASS2) && !defined(APP_CONFIG_PASS1_INCLUDED)
#ifdef CONFIG_CACHE_PASS2
# define APP_CONFIG_PASS2_INCLUDED
#else
# define APP_CONFIG_PASS1_INCLUDED
#endif

#include <config/ConfigCache.h>

REFLECTION_BEGIN(ConfPlayerBase)
	VAR_STR(nick, "Player")
	VAR_INT(team,         0)
	VAR_STR(skin, "neutral")
	VAR_STR(platform_class, "default")
REFLECTION_END()

REFLECTION_BEGIN_(ConfPlayerLocal, ConfPlayerBase)
	VAR_STR(profile,   "")
REFLECTION_END()

REFLECTION_BEGIN_(ConfPlayerAI, ConfPlayerBase)
	VAR_INT(level,  2)
REFLECTION_END()

REFLECTION_BEGIN(DMCampaignMapDesc)
	VAR_STR(map_name, "")
	VAR_FLOAT(timelimit, 10)  HELPSTRING("minutes")
	VAR_INT(fraglimit, 10)
	VAR_ARRAY(bot_names, nullptr)
REFLECTION_END()

REFLECTION_BEGIN(DMCampaignTier)
	VAR_STR(title, "")
	VAR_ARRAY(maps, nullptr)
REFLECTION_END()

REFLECTION_BEGIN(DMCampaign)
	VAR_ARRAY(tiers, nullptr)
	VAR_TABLE(bots, nullptr)
REFLECTION_END()

REFLECTION_BEGIN(AppConfig)
	VAR_ARRAY(sp_tiersprogress, nullptr)
	VAR_REFLECTION(sp_playerinfo, ConfPlayerLocal)
	VAR_INT(sp_difficulty, 0)
REFLECTION_END()

bool IsTierComplete(AppConfig &appConfig, const DMCampaign &dmCampaign, int tierIndex);

#endif
