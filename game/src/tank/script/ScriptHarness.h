#pragma once
#include "sPickup.h"
#include "sPlayer.h"
#include "sRigidBodyStatic.h"
#include "sTrigger.h"
#include <gc/WorldEvents.h>
#include "gclua/lgcmod.h"
#include <memory>

class SaveFile;
class World;
struct ScriptEnvironment;
struct lua_State;
namespace FS {
	class Stream;
}

class ScriptHarness
	: ObjectListener<World>
{
public:
	explicit ScriptHarness(World &world);
	~ScriptHarness();
	lua_State* GetLuaState() { return _L; }
	
	void Step(float dt);
	
	void Serialize(SaveFile &f);
	void Deserialize(SaveFile &f);
	
private:
	ScriptEnvironment _se;
	World &_world;
	lua_State *_L;
	
	sPickup _sPickup;
	sPlayer _sPlayer;
	sRigidBodyStatic _sRigidBodyStatic;
	sTrigger _sTrigger;

	// ObjectListener<World>
	virtual void OnGameStarted() override;
	virtual void OnGameFinished() override {}
	virtual void OnKill(GC_Object &) override {}
	virtual void OnNewObject(GC_Object &) override {}
};
