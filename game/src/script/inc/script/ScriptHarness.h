#pragma once
#include "detail/sPickup.h"
#include "detail/sPlayer.h"
#include "detail/sRigidBodyStatic.h"
#include "detail/sTrigger.h"
#include <gc/WorldEvents.h>
#include <memory>

class SaveFile;
class World;
struct ScriptMessageSink;
struct lua_State;
namespace FS {
	class Stream;
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
