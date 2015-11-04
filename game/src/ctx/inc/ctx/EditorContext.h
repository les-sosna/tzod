#pragma once

#include "GameContext.h"
#include <memory>

class World;
namespace FS
{
	class Stream;
}

class EditorContext : public GameContextBase
{
public:
	EditorContext(FS::Stream &stream);
	EditorContext(int width, int height);
	virtual ~EditorContext();

	// GameContextBase
	World& GetWorld() override { return *_world; }
	void Step(float dt) override;

private:
	std::unique_ptr<World> _world;
};
