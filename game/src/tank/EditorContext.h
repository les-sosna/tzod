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
	EditorContext(float width, float height);
	virtual ~EditorContext();
	
	// GameContextBase
	virtual World& GetWorld() override { return *_world; }
	virtual void Step(float dt) override;
	
private:
	std::unique_ptr<World> _world;
};
