#pragma once
#include "sPickup.h"
#include "sPlayer.h"
#include "sRigidBodyStatic.h"
#include "sTrigger.h"
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
	: ObjectListener<World>
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
	
	sPickup _sPickup;
	sPlayer _sPlayer;
	sRigidBodyStatic _sRigidBodyStatic;
	sTrigger _sTrigger;

	// ObjectListener<World>
	virtual void OnGameStarted() override;
	virtual void OnGameMessage(const char *) override {}
};
