#pragma once
#include "Gameplay.h"
#include <gc/WorldEvents.h>

class World;
class WorldController;
struct GameListener;

class Deathmatch
	: public Gameplay
	, private ObjectListener<GC_RigidBodyStatic>
{
public:
	Deathmatch(World &world, WorldController &worldController, GameListener &gameListener);
	virtual ~Deathmatch();

	int GetFragLimit() const { return _fragLimit; }
	void SetFragLimit(int fragLimit) { _fragLimit = fragLimit; }
	void SetTimeLimit(float timeLimit) { _timeLimit = timeLimit; }

	// Gameplay
	void Step() override;
	float GetGameEndTime() const override;
	float GetTimeLimit() const override { return _timeLimit; }
	void Serialize(SaveFile &f) override;

private:
	World &_world;
	WorldController &_worldController;
	GameListener &_gameListener;
	int _fragLimit = 0;
	float _timeLimit = 0;
	int _maxScore = 0;
	float _maxScoreTime = 0;

	// ObjectListener<GC_RigidBodyStatic>
	void OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;
	void OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd) override {}
};
