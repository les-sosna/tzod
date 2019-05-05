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
	, _difficulty(settings.difficulty)
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

Gameplay* GameContext::GetGameplay() const
{
	return _gameplay.get();
}

void GameContext::Step(float dt, AppConfig &appConfig, bool *outConfigChanged)
{
	if (IsGameplayActive())
		_gameplayTime += dt;

	if (IsWorldActive())
	{
		_worldController->SendControllerStates(_aiManager->ComputeAIState(*_world, dt));
		_world->Step(dt);
		_scriptHarness->Step(dt);
	}
}

bool GameContext::IsGameplayActive() const
{
	for (auto player : _worldController->GetLocalPlayers())
		if (!player->GetIsActive())
			return false;
	return true;
}

bool GameContext::IsWorldActive() const
{
	return (_world->GetTime() < _gameplay->GetGameOverTime()) && IsGameplayActive();
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

int GameContextCampaignDM::GetRating() const
{
	return IsVictory() ? ((int)GetDifficulty() + 1) : 0;
}

bool GameContextCampaignDM::IsVictory() const
{
	int maxBotScore = INT_MIN;
	for (auto botPlayer : GetWorldController().GetAIPlayers())
		maxBotScore = std::max(maxBotScore, botPlayer->GetScore());

	for (auto player : GetWorldController().GetLocalPlayers())
	{
		if (player->GetScore() > maxBotScore)
			return true;
	}
	return false;
}

void GameContextCampaignDM::Step(float dt, AppConfig &appConfig, bool *outConfigChanged)
{
	GameContext::Step(dt, appConfig, outConfigChanged);
	auto gameplay = GetGameplay();
	assert(gameplay);
	if (gameplay->GetGameOverTime() <= GetWorld().GetTime() &&
		_campaignTier >= 0 && _campaignMap >= 0 && IsVictory())
	{
		appConfig.sp_tiersprogress.EnsureIndex(_campaignTier);
		ConfVarArray &tierprogress = appConfig.sp_tiersprogress.GetArray(_campaignTier);
		tierprogress.EnsureIndex(_campaignMap);
		int currentRating = tierprogress.GetNum(_campaignMap).GetInt();
		int rating = GetRating();
		if (rating > currentRating)
		{
			tierprogress.SetNum(_campaignMap, rating);
			*outConfigChanged = true;
		}
	}
}
