#pragma once
#include "gc/WorldEvents.h"
#include <memory>

class SaveFile;
class World;
struct ScriptEnvironment;
struct lua_State;
namespace FS {
	class Stream;
}

class ScriptHarness
	: ObjectListener<GC_Trigger>
	, ObjectListener<GC_RigidBodyStatic>
	, ObjectListener<GC_Pickup>
	, ObjectListener<GC_Player>
	, ObjectListener<World>
{
public:
	ScriptHarness(World &world, ScriptEnvironment &se);
	~ScriptHarness();
	lua_State* GetLuaState() { return _L; }
	
	void Step(float dt);
	
	void Serialize(std::shared_ptr<FS::Stream> stream, SaveFile &f);
	void Deserialize(std::shared_ptr<FS::Stream> stream, SaveFile &f);
	
private:
	World &_world;
	lua_State *_L;
	
	// ObjectListener<GC_Trigger>
	virtual void OnEnter(GC_Trigger &obj, GC_Vehicle &vehicle) override;
	virtual void OnLeave(GC_Trigger &obj) override;
	
	// ObjectListener<GC_RigidBodyStatic>
	virtual void OnDestroy(GC_RigidBodyStatic &obj) override;
	virtual void OnDamage(GC_RigidBodyStatic &obj, GC_Actor *from) override;
	
	// ObjectListener<GC_Pickup>
	virtual void OnPickup(GC_Pickup &obj, GC_Actor &actor) override;
	
	// ObjectListener<GC_Player>
	virtual void OnRespawn(GC_Player &obj, GC_Vehicle &vehicle) override;
	virtual void OnDie(GC_Player &obj) override;

	// ObjectListener<GC_Object>
	virtual void OnCreate(GC_Object &) override {}
	virtual void OnKill(GC_Object &) override {}

	// ObjectListener<World>
	virtual void OnGameStarted() override;
	virtual void OnGameMessage(const char *) override {}
};
