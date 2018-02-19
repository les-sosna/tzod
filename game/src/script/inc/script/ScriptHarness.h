#pragma once
#include "detail/sPickup.h"
#include "detail/sPlayer.h"
#include "detail/sRigidBodyStatic.h"
#include "detail/sTrigger.h"
#include <gc/WorldEvents.h>
#include <luaetc/LuaDeleter.h>
#include <memory>

class SaveFile;
class World;
struct ScriptMessageSink;
struct lua_State;
namespace FS {
	struct Stream;
}

class ScriptHarness
	: ObjectListener<World>
{
public:
	explicit ScriptHarness(World &world, ScriptMessageSink &messageSink);
	~ScriptHarness();

	void Step(float dt);

	void Serialize(SaveFile &f);
	void Deserialize(SaveFile &f);

private:
	World &_world;
	ScriptMessageSink &_messageSink;
	std::unique_ptr<lua_State, LuaStateDeleter> _L;

	sPickup _sPickup;
	sPlayer _sPlayer;
	sRigidBodyStatic _sRigidBodyStatic;
	sTrigger _sTrigger;

	// ObjectListener<World>
	void OnGameStarted() override;
	void OnGameFinished() override {}
	void OnKill(GC_Object &) override {}
	void OnNewObject(GC_Object &) override {}
};
