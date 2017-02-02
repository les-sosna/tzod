#include "inc/as/AppController.h"
#include "inc/as/AppConfig.h"
#include "inc/as/AppConstants.h"
#include "inc/as/AppState.h"
#include <ctx/Gameplay.h>
#include <ctx/GameContext.h>
#include <fs/FileSystem.h>
#include <algorithm>

static PlayerDesc GetPlayerDescFromConf(const ConfPlayerBase &p)
{
	PlayerDesc result;
	result.nick = p.nick.Get();
	result.cls = p.platform_class.Get();
	result.skin = p.skin.Get();
	result.team = p.team.GetInt();
	return result;
}

static DMSettings GetCampaignDMSettings(AppConfig &appConfig, DMCampaign &dmCampaign, unsigned int tier, unsigned int map)
{
	DMSettings settings;

	settings.players.push_back(GetPlayerDescFromConf(appConfig.sp_playerinfo));

	DMCampaignTier tierDesc(&dmCampaign.tiers.GetTable(tier));
	DMCampaignMapDesc mapDesc(&tierDesc.maps.GetTable(map));

	for (size_t i = 0; i < mapDesc.bot_names.GetSize(); ++i)
	{
		ConfPlayerAI p(&dmCampaign.bots.GetTable(mapDesc.bot_names.GetStr(i).Get()));
		settings.bots.push_back(GetPlayerDescFromConf(p));
	}

	settings.timeLimit = mapDesc.timelimit.GetFloat() * 60;
	settings.fragLimit = mapDesc.fraglimit.GetInt();

	return settings;
}


AppController::AppController(FS::FileSystem &fs)
    : _fs(fs)
{
}

void AppController::Step(AppState &appState, AppConfig &appConfig, float dt)
{
	if (GameContextBase *gc = appState.GetGameContext())
	{
		gc->Step(dt);

		if (auto gameplay = gc->GetGameplay())
		{
			if (gameplay->IsGameOver())
			{
				if (auto campaignGC = dynamic_cast<GameContext*>(gc))
				{
					if (campaignGC->GetCampaignTier() >= 0 && campaignGC->GetCampaignMap() >= 0)
					{
						appConfig.sp_tiersprogress.EnsureIndex(campaignGC->GetCampaignTier());
						ConfVarArray &tierprogress = appConfig.sp_tiersprogress.GetArray(campaignGC->GetCampaignTier());
						tierprogress.EnsureIndex(campaignGC->GetCampaignMap());
						int currentRating = tierprogress.GetNum(campaignGC->GetCampaignMap()).GetInt();
						tierprogress.SetNum(campaignGC->GetCampaignMap(), std::max(currentRating, gameplay->GetRating()));
					}
				}
			}
		}
	}
}

//void AppController::NewGameDM(TzodApp &app, const std::string &mapName, const DMSettings &settings)
//{
//    std::string path = std::string(DIR_MAPS) + '/' + mapName + ".map";
//    std::shared_ptr<FS::Stream> stream = _fs.Open(path)->QueryStream();
//    std::unique_ptr<GameContext> gc(new GameContext(*stream, settings));
//    appState.SetGameContext(std::move(gc));
//}

void AppController::StartDMCampaignMap(AppState &appState, AppConfig &appConfig, DMCampaign &dmCampaign, unsigned int tier, unsigned int map)
{
	DMCampaignTier tierDesc(&dmCampaign.tiers.GetTable(tier));
	DMCampaignMapDesc mapDesc(&tierDesc.maps.GetTable(map));

	std::string path = std::string(DIR_MAPS) + '/' + mapDesc.map_name.Get() + ".map";
	std::shared_ptr<FS::Stream> stream = _fs.Open(path)->QueryStream();
	DMSettings settings = GetCampaignDMSettings(appConfig, dmCampaign, tier, map);
	std::unique_ptr<GameContext> gc(new GameContext(*stream, settings, tier, map));
	appState.SetGameContext(std::move(gc));
}
