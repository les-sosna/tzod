#include "GameContext.h"

GameContext::GameContext(FS::FileSystem &fs,
						 ThemeManager &themeManager,
						 TextureManager &textureManager,
						 std::function<void()> exitCommand)
	: _gameEventsBroadcaster()
	, _deathmatch(_world, _gameEventsBroadcaster)
	, _scriptEnvironment(ScriptEnvironment{ // msvc workaround
		_world,
		_gameEventsBroadcaster,
		fs,
		themeManager,
		textureManager,
		std::move(exitCommand) })
	, _scriptHarness(_world, _scriptEnvironment)
{
}

void GameContext::Step(float dt)
{
	_world.Step(dt);
	_scriptHarness.Step(dt);
#ifndef NOSOUND
	if( _scriptEnvironment.music )
	{
		_scriptEnvironment.music->HandleBufferFilling();
	}
#endif
}
