#include "inc/ctx/Deathmatch.h"
#include "inc/ctx/GameContext.h"
#include "inc/ctx/WorldController.h"
#include "AIManager.h"
#include <gc/Player.h>
#include <gc/SaveFile.h>
#include <gc/World.h>
#include <gc/WorldCfg.h>
#include <script/ScriptHarness.h>

#define AI_MAX_LEVEL   4U

GameContext::GameContext(std::unique_ptr<World> world, const DMSettings &settings, int campaignTier, int campaignMap)
	: _campaignTier(campaignTier)
	, _campaignMap(campaignMap)
	, _world(std::move(world))
{
	_world->Seed(rand());
	_aiManager.reset(new AIManager(*_world));

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

		_aiManager->AssignAI(&player, "123");
//        ai->SetAILevel(std::max(0U, std::min(AI_MAX_LEVEL, p.level.GetInt())));
	}

	_worldController.reset(new WorldController(*_world));

	std::unique_ptr<Deathmatch> gameplay(new Deathmatch(*_world, *_worldController, _gameEventsBroadcaster));
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

void GameContext::Step(float dt)
{
	if (IsActive())
	{
		_worldController->SendControllerStates(_aiManager->ComputeAIState(*_world, dt));
		_world->Step(dt);
		_scriptHarness->Step(dt);
	}
}

bool GameContext::IsActive() const
{
	bool isActive = !_gameplay->IsGameOver();

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
	int width = (int) WIDTH(_world->_bounds) / CELL_SIZE;
	int height = (int) HEIGHT(_world->_bounds) / CELL_SIZE;
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

	_world.reset(new World(RectRB{ width, height }));
	_world->Deserialize(f);

	// TODO: deserialize world controller
	_worldController.reset(new WorldController(*_world));

	// TODO: restore gameplay type
	_gameplay.reset(new Deathmatch(*_world, *_worldController, _gameEventsBroadcaster));
	_gameplay->Serialize(f);

	_scriptHarness.reset(new ScriptHarness(*_world, _scriptMessageBroadcaster));
	_scriptHarness->Deserialize(f);
}
