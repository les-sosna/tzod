#include "inc/as/AppController.h"
#include "inc/as/AppConstants.h"
#include "inc/as/AppState.h"
#include <ctx/AppConfig.h>
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

	settings.difficulty = static_cast<AIDiffuculty>(std::clamp(appConfig.sp_difficulty.GetInt(), 0, 3));
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
		gc->Step(dt, appConfig, outConfigChanged);
	}
}

//void AppController::NewGameDM(TzodApp &app, const std::string &mapName, const DMSettings &settings)
//{
//    std::string path = std::string(DIR_MAPS) + '/' + mapName + ".tzod";
//    std::shared_ptr<FS::Stream> stream = _fs.Open(path)->QueryStream();
//    std::unique_ptr<GameContext> gc(new GameContext(*stream, settings));
//    appState.SetGameContext(std::move(gc));
//}

void AppController::StartDMCampaignMap(AppState& appState, AppConfig& appConfig, DMCampaign& dmCampaign, unsigned int tier, unsigned int map)
{
	DMCampaignTier tierDesc(&dmCampaign.tiers.GetTable(tier));
	DMCampaignMapDesc mapDesc(&tierDesc.maps.GetTable(map));

	while (appState.GetGameContext())
		appState.PopGameContext();

	auto world = _worldCache->CheckoutCachedWorld(_fs, mapDesc.map_name.Get());
	DMSettings settings = GetCampaignDMSettings(appConfig, dmCampaign, tier, map);
	appState.PushGameContext(std::make_shared<GameContextCampaignDM>(std::move(world), settings, tier, map));
}

void AppController::SetEditorMode(AppState &appState, bool editorMode)
{
	if (editorMode)
	{
		appState.PopGameContext();
	}
	else
	{
		auto editorContext = std::dynamic_pointer_cast<EditorContext>(appState.GetGameContext());
		assert(editorContext);

		auto fileName = "user/current.tzod";
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

		appState.PushGameContext(std::make_shared<GameContext>(std::move(world), settings));
	}
}

void AppController::StartNewMapEditor()
{

}
