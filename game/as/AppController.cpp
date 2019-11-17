#define _CRT_SECURE_NO_WARNINGS
#include "inc/as/AppController.h"
#include "inc/as/AppConstants.h"
#include "inc/as/AppState.h"
#include "inc/as/MapCollection.h"
#include <ctx/AppConfig.h>
#include <ctx/EditorContext.h>
#include <ctx/Gameplay.h>
#include <ctx/GameContext.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <algorithm>
#include <time.h>

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

void AppController::StartDMCampaignMap(AppState& appState, MapCollection &mapCollection, AppConfig& appConfig, DMCampaign& dmCampaign, unsigned int tier, unsigned int map)
{
	DMCampaignTier tierDesc(&dmCampaign.tiers.GetTable(tier));
	DMCampaignMapDesc mapDesc(&tierDesc.maps.GetTable(map));

	while (appState.GetGameContext())
		appState.PopGameContext();

	auto world = mapCollection.ExtractCachedWorld(_fs, mapDesc.map_name.Get());
	DMSettings settings = GetCampaignDMSettings(appConfig, dmCampaign, tier, map);
	appState.PushGameContext(std::make_shared<GameContextCampaignDM>(std::move(world), settings, tier, map));
}

void AppController::PlayCurrentMap(AppState &appState, MapCollection& mapCollection)
{
	auto editorContext = std::dynamic_pointer_cast<EditorContext>(appState.GetGameContext());
	assert(editorContext);

	SaveCurrentMap(*editorContext, mapCollection);

	auto world = mapCollection.ExtractCachedWorld(_fs, editorContext->GetMapName());

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

void AppController::StartNewMapEditor(AppState& appState, MapCollection& mapCollection, int width, int height, std::string_view existingMapNameOptional)
{
	std::string mapName;
	std::shared_ptr<FS::Stream> stream;
	if (existingMapNameOptional.empty())
	{
		// generate new unique map name

		auto t = time(nullptr);
		auto tm = gmtime(&t);
		
		char str[256];
		snprintf(str, sizeof(str), "!%04d%02d%02d%02d%02d%02d-", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		mapName = str;

		static const std::string_view abc = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		for (int i = 0; i < 4; i++)
			mapName.push_back(abc[rand() % abc.size()]);
	}
	else
	{
		mapName = existingMapNameOptional;
		stream = mapCollection.QueryReadStream(_fs, mapName);
	}
	appState.PushGameContext(std::make_unique<EditorContext>(std::move(mapName), width, height, stream.get()));
}

void AppController::SaveAndExitEditor(AppState& appState, MapCollection& mapCollection)
{
	auto editorContext = std::dynamic_pointer_cast<EditorContext>(appState.GetGameContext());
	assert(editorContext);
	SaveCurrentMap(*editorContext, mapCollection);
	appState.PopGameContext();
}

void AppController::SaveCurrentMap(EditorContext& editorContext, MapCollection& mapCollection)
{
	auto fileName = std::string(editorContext.GetMapName()) + ".tzod";
	editorContext.GetWorld().Export(*_fs.GetFileSystem("user")->GetFileSystem(DIR_MAPS, true)->Open(fileName, FS::ModeWrite)->QueryStream());
	mapCollection.AddOrUpdateUserMap(std::string(editorContext.GetMapName()));
}
