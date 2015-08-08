#include "inc/app/Deathmatch.h"
#include "inc/app/GameContext.h"
#include "inc/app/WorldController.h"
#include "AIManager.h"
#include <fs/FileSystem.h>
#include <gc/Player.h>
#include <gc/SaveFile.h>
#include <gc/World.h>
#include <gc/WorldCfg.h>
#include <script/ScriptHarness.h>
#include <MapFile.h>

GameContext::GameContext(FS::Stream &map, const DMSettings &settings)
{
	MapFile file(map, false);
	
	int width, height;
	if (!file.getMapAttribute("width", width) ||
	    !file.getMapAttribute("height", height))
	{
		throw std::runtime_error("unknown map size");
	}

	std::string theme;
	file.getMapAttribute("theme", theme);
//	themeManager.ApplyTheme(themeManager.FindTheme(theme), tm);
	
	_world.reset(new World(width, height));
	_aiManager.reset(new AIManager(*_world));
	
	_world->Seed(rand());
	_world->Import(file);

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

	_gameplay.reset(new Deathmatch(*_world, _gameEventsBroadcaster));
	_scriptHarness.reset(new ScriptHarness(*_world, _scriptMessageBroadcaster));
	_worldController.reset(new WorldController(*_world));
}

GameContext::~GameContext()
{
}

void GameContext::Step(float dt)
{
	_worldController->SendControllerStates(_aiManager->ComputeAIState(*_world, dt));
	_world->Step(dt);	
	_scriptHarness->Step(dt);
}

void GameContext::Serialize(FS::Stream &stream)
{
	SaveFile f(stream, false);

	int version = VERSION;
	int width = (int) _world->_sx / CELL_SIZE;
	int height = (int) _world->_sy / CELL_SIZE;
	f.Serialize(version);
	f.Serialize(width);
	f.Serialize(height);
//	f.Serialize(_infoTheme);
	
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
//	f.Serialize(_infoTheme);

	if( VERSION != version )
		throw std::runtime_error("invalid version");

	_world.reset(new World(width, height));
	_world->Deserialize(f);
	
	// TODO: restore gameplay type
	_gameplay.reset(new Deathmatch(*_world, _gameEventsBroadcaster));
	_gameplay->Serialize(f);
	
//	themeManager.ApplyTheme(themeManager.FindTheme(_world->_infoTheme), tm);

	_scriptHarness.reset(new ScriptHarness(*_world, _scriptMessageBroadcaster));
	_scriptHarness->Deserialize(f);
}
