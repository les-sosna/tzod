#pragma once

class World;
struct Gameplay;

struct GameContextBase
{
	virtual ~GameContextBase() {}
	virtual World& GetWorld() = 0;
	virtual Gameplay* GetGameplay() = 0;
	virtual void Step(float dt) = 0;
};
