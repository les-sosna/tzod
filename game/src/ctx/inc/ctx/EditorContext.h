#pragma once

#include "GameContext.h"
#include <memory>

class World;
namespace FS
{
	struct Stream;
}

class EditorContext : public GameContextBase
{
public:
	EditorContext(int width, int height, FS::Stream *stream = nullptr);
	virtual ~EditorContext();

	// GameContextBase
	World& GetWorld() override { return *_world; }
	Gameplay* GetGameplay() override { return nullptr; }
	void Step(float dt) override;

private:
	std::unique_ptr<World> _world;
};
