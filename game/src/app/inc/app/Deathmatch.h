#pragma once
#include <gc/WorldEvents.h>

class SaveFile;
class World;
struct GameListener;

struct Gameplay
{
	virtual ~Gameplay() {}
	virtual void Step() = 0;
	virtual bool IsGameOver() = 0;
	virtual void Serialize(SaveFile &f) = 0;
};

class Deathmatch
	: public Gameplay
	, private ObjectListener<GC_RigidBodyStatic>
{
public:
	Deathmatch(World &world, GameListener &gameListener);
	
	int GetFragLimit() const { return _fragLimit; }
	float GetTimeLimit() const { return _timeLimit; }
	
	// Gameplay
	virtual ~Deathmatch();
	virtual void Step() override;
	virtual bool IsGameOver() override;
	virtual void Serialize(SaveFile &f) override;
	
private:
	World &_world;
	GameListener &_gameListener;
	int _fragLimit;
	float _timeLimit;
	
	// ObjectListener<GC_RigidBodyStatic>
	virtual void OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;
	virtual void OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd) override {}
};
