#pragma once
#include <gc/WorldEvents.h>

class SaveFile;
class World;
class WorldController;
struct GameListener;

struct Gameplay
{
	virtual ~Gameplay() {}
	virtual void Step() = 0;
	virtual bool IsGameOver() const = 0;
	virtual void Serialize(SaveFile &f) = 0;
};

class Deathmatch
	: public Gameplay
	, private ObjectListener<GC_RigidBodyStatic>
{
public:
	Deathmatch(World &world, WorldController &worldController, GameListener &gameListener);
	virtual ~Deathmatch();

	int GetFragLimit() const { return _fragLimit; }
	void SetFragLimit(int fragLimit) { _fragLimit = fragLimit; }

	float GetTimeLimit() const { return _timeLimit; }
	void SetTimeLimit(float timeLimit) { _timeLimit = timeLimit; }

	int GetRating() const;

	// Gameplay
	void Step() override;
	bool IsGameOver() const override;
	void Serialize(SaveFile &f) override;

private:
	World &_world;
	WorldController &_worldController;
	GameListener &_gameListener;
	int _fragLimit = 0;
	float _timeLimit = 0;
	int _maxScore = 0;

	// ObjectListener<GC_RigidBodyStatic>
	void OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;
	void OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd) override {}
};
