#include "inc/as/AppController.h"
#include "inc/as/AppConfig.h"
#include "inc/as/AppConstants.h"
#include "inc/as/AppState.h"
#include <ctx/EditorContext.h>
#include <ctx/Gameplay.h>
#include <ctx/GameContext.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <gc/WorldCache.h>
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
	, _worldCache(new WorldCache())
{
}

AppController::~AppController()
{
}

void AppController::Step(AppState &appState, AppConfig &appConfig, float dt, bool *outConfigChanged)
{
	if (GameContextBase *gc = appState.GetGameContext().get())
	{
		gc->Step(dt);

		if (auto gameplay = gc->GetGameplay())
		{
			if (gameplay->GetGameOverTime() <= gc->GetWorld().GetTime())
			{
				if (auto campaignGC = dynamic_cast<GameContextCampaignDM*>(gc))
				{
					if (campaignGC->GetCampaignTier() >= 0 && campaignGC->GetCampaignMap() >= 0)
					{
						appConfig.sp_tiersprogress.EnsureIndex(campaignGC->GetCampaignTier());
						ConfVarArray &tierprogress = appConfig.sp_tiersprogress.GetArray(campaignGC->GetCampaignTier());
						tierprogress.EnsureIndex(campaignGC->GetCampaignMap());
						int currentRating = tierprogress.GetNum(campaignGC->GetCampaignMap()).GetInt();
						if (gameplay->GetRating() > currentRating)
						{
							tierprogress.SetNum(campaignGC->GetCampaignMap(), gameplay->GetRating());
							*outConfigChanged = true;
						}
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

	auto world = _worldCache->CheckoutCachedWorld(_fs, mapDesc.map_name.Get());
	DMSettings settings = GetCampaignDMSettings(appConfig, dmCampaign, tier, map);
	appState.SetGameContext(std::make_shared<GameContextCampaignDM>(std::move(world), settings, tier, map));

	_savedEditorContext.reset();
}

void AppController::SetEditorMode(AppState &appState, bool editorMode)
{
	if (editorMode)
	{
		assert(_savedEditorContext);
		appState.SetGameContext(std::move(_savedEditorContext));
		_savedEditorContext.reset();
	}
	else
	{
//		assert(!_savedEditorContext);

		auto editorContext = std::dynamic_pointer_cast<EditorContext>(appState.GetGameContext());
		assert(editorContext);

		auto fileName = "user/current.map";
		editorContext->GetWorld().Export(*_fs.Open(fileName, FS::ModeWrite)->QueryStream());

		std::unique_ptr<World> world = LoadMapUncached(*_fs.GetFileSystem("user"), "current");

		PlayerDesc player;
		player.nick = "Player";
		player.skin = "red";
		player.cls = "default";
		player.team = 0;

		PlayerDesc player2;
		player2.nick = "Player2";
		player2.skin = "yellow";
		player2.cls = "default";
		player2.team = 0;

		PlayerDesc bot;
		bot.nick = "Enemy";
		bot.skin = "FBI Tank";
		bot.cls = "default";
		bot.team = 0;

		DMSettings settings;
		settings.players.push_back(player);
		settings.players.push_back(player2);
		//settings.bots.push_back(bot);
		settings.timeLimit = 300;

		appState.SetGameContext(std::make_shared<GameContext>(std::move(world), settings));

		_savedEditorContext = editorContext;
	}
}

void AppController::StartNewMapEditor()
{

}
