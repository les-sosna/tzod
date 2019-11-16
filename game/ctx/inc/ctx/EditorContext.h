#pragma once

#include "GameContext.h"
#include <math/MyMath.h>
#include <memory>

class World;
namespace FS
{
	struct Stream;
}

class EditorContext : public GameContextBase
{
public:
	EditorContext(std::string mapName, int width, int height, FS::Stream *stream);
	virtual ~EditorContext();

	std::string_view GetMapName() const { return _mapName; }
	FRECT GetOriginalBounds() const { return _originalBounds; }

	// GameContextBase
	World& GetWorld() override { return *_world; }
	Gameplay* GetGameplay() const override { return nullptr; }
	void Step(float dt, AppConfig &appConfig, bool *outConfigChanged) override;
	bool IsWorldActive() const override { return false; }

private:
	std::string _mapName;
	std::unique_ptr<World> _world;
	FRECT _originalBounds = {};
};
