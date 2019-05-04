#include "inc/ctx/AIManager.h"
#include "inc/ctx/AppConfig.h"
#include "inc/ctx/Deathmatch.h"
#include "inc/ctx/GameContext.h"
#include "inc/ctx/WorldController.h"
#include <gc/Player.h>
#include <gc/SaveFile.h>
#include <gc/World.h>
#include <gc/WorldCfg.h>
#include <script/ScriptHarness.h>

GameContext::GameContext(std::unique_ptr<World> world, const DMSettings &settings)
	: _world(std::move(world))
	, _aiManager(std::make_unique<AIManager>(*_world))
{
	_world->Seed(rand());

	for( const PlayerDesc &pd: settings.players )
	{
		auto &player = _world->New<GC_Player>();
		player.SetIsHuman(true);
		player.SetClass(pd.cls);
		player.SetNick(pd.nick);
		player.SetSkin(pd.skin);
		player.SetTeam(pd.team);

//		_inputMgr->AssignController(&player, p.profile.Get());
	}

	for( const PlayerDesc &pd: settings.bots )
	{
		auto &player = _world->New<GC_Player>();
		player.SetIsHuman(false);
		player.SetClass(pd.cls);
		player.SetNick(pd.nick);
		player.SetSkin(pd.skin);
		player.SetTeam(pd.team);

		_aiManager->AssignAI(&player, settings.difficulty);
	}

	_worldController.reset(new WorldController(*_world));

	auto gameplay = std::make_unique<Deathmatch>(*_world, *_worldController, _gameEventsBroadcaster);
	gameplay->SetFragLimit(settings.fragLimit);
	gameplay->SetTimeLimit(settings.timeLimit);
	_gameplay = std::move(gameplay);

	_scriptHarness.reset(new ScriptHarness(*_world, _scriptMessageBroadcaster));
}

GameContext::~GameContext()
{
}

Gameplay* GameContext::GetGameplay()
{
	return _gameplay.get();
}

void GameContext::Step(float dt, AppConfig &appConfig, bool *outConfigChanged)
{
	_gameplayTime += dt;
	if (IsWorldActive())
	{
		_worldController->SendControllerStates(_aiManager->ComputeAIState(*_world, dt));
		_world->Step(dt);
		_scriptHarness->Step(dt);
	}
}

bool GameContext::IsWorldActive() const
{
	bool isActive = _world->GetTime() < _gameplay->GetGameOverTime();

	if (isActive)
	{
		for (auto player : _worldController->GetLocalPlayers())
		{
			if (!player->GetIsActive())
			{
				isActive = false;
				break;
			}
		}
	}

	return isActive;
}

void GameContext::Serialize(FS::Stream &stream)
{
	SaveFile f(stream, false);

	int version = VERSION;
	int width = (int) WIDTH(_world->GetBounds()) / WORLD_BLOCK_SIZE;
	int height = (int) HEIGHT(_world->GetBounds()) / WORLD_BLOCK_SIZE;
	f.Serialize(version);
	f.Serialize(width);
	f.Serialize(height);

	_world->Serialize(f);
	_gameplay->Serialize(f);
	_scriptHarness->Serialize(f);
}

void GameContext::Deserialize(FS::Stream &stream)
{
	SaveFile f(stream, true);

	int version = 0;
	int width = 0;
	int height = 0;
	f.Serialize(version);
	f.Serialize(width);
	f.Serialize(height);

	if( VERSION != version )
		throw std::runtime_error("invalid version");

	_world.reset(new World(RectRB{ width, height }, true /* initField */));
	_world->Serialize(f);

	// TODO: deserialize world controller
	_worldController.reset(new WorldController(*_world));

	// TODO: restore gameplay type
	_gameplay.reset(new Deathmatch(*_world, *_worldController, _gameEventsBroadcaster));
	_gameplay->Serialize(f);

	_scriptHarness.reset(new ScriptHarness(*_world, _scriptMessageBroadcaster));
	_scriptHarness->Deserialize(f);
}

///////////////////////////////////////////////////////

GameContextCampaignDM::GameContextCampaignDM(std::unique_ptr<World> world, const DMSettings &settings, int campaignTier, int campaignMap)
	: GameContext(std::move(world), settings)
	, _campaignTier(campaignTier)
	, _campaignMap(campaignMap)
{
}

void GameContextCampaignDM::Step(float dt, AppConfig &appConfig, bool *outConfigChanged)
{
	GameContext::Step(dt, appConfig, outConfigChanged);
	auto gameplay = GetGameplay();
	assert(gameplay);
	if (gameplay->GetGameOverTime() <= GetWorld().GetTime() &&
		GetCampaignTier() >= 0 && GetCampaignMap() >= 0)
	{
		appConfig.sp_tiersprogress.EnsureIndex(GetCampaignTier());
		ConfVarArray &tierprogress = appConfig.sp_tiersprogress.GetArray(GetCampaignTier());
		tierprogress.EnsureIndex(GetCampaignMap());
		int currentRating = tierprogress.GetNum(GetCampaignMap()).GetInt();
		if (gameplay->GetRating() > currentRating)
		{
			tierprogress.SetNum(GetCampaignMap(), gameplay->GetRating());
			*outConfigChanged = true;
		}
	}
}
