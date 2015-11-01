#include <gc/VehicleState.h>
#include <gc/WorldEvents.h>
#include <gc/detail/PtrList.h>

#include <map>
#include <memory>
#include <string>

class AIController;
class GC_Object;
class GC_Player;
class GC_Vehicle;
class World;

class AIManager
	: private ObjectListener<GC_Player>
	, private ObjectListener<World>
{
public:
	AIManager(World &world);
	~AIManager();
    void AssignAI(GC_Player *player, std::string profile);
    void FreeAI(GC_Player *player);

	typedef std::map<PtrList<GC_Object>::id_type, VehicleState> ControllerStateMap;
	ControllerStateMap ComputeAIState(World &world, float dt);

private:
	std::map<GC_Player *, std::pair<std::string, std::unique_ptr<AIController>>> _aiControllers;
	World &_world;

	// ObjectListener<GC_Player>
	void OnRespawn(GC_Player &obj, GC_Vehicle &vehicle) override;
	void OnDie(GC_Player &obj) override;

	// ObjectListener<World>
	void OnKill(GC_Object &obj) override;
	void OnNewObject(GC_Object &) override {}
	void OnGameStarted() override {}
	void OnGameFinished() override {}
};

