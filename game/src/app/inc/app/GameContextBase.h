#pragma once

class World;

struct GameContextBase
{
	virtual ~GameContextBase() {}
	virtual World& GetWorld() = 0;
	virtual void Step(float dt) = 0;
};
