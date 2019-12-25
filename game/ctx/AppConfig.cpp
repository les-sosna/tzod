#include <config/ConfigBase.h>

// first pass to define the structure
#include "inc/ctx/AppConfig.h"

// second pass to implement initialize function
#define CONFIG_CACHE_PASS2
#include "inc/ctx/AppConfig.h"
#undef CONFIG_CACHE_PASS2

/////////////////////////////////////////////
// Utility access functions

bool IsTierComplete(AppConfig &appConfig, const DMCampaign &dmCampaign, int tierIndex)
{
	if (tierIndex >= appConfig.sp_tiersprogress.GetSize())
		return false;

	bool result = true;
	ConfVarArray &tierProgress = appConfig.sp_tiersprogress.GetArray(tierIndex);

	DMCampaignTier tierDesc(&dmCampaign.tiers.GetTable(tierIndex));
	for (auto mapIndex = tierDesc.maps.GetSize(); mapIndex--;)
	{
		result &= (mapIndex < tierProgress.GetSize() && tierProgress.GetNum(mapIndex).GetInt() > 0);
	}

	return result;
}
