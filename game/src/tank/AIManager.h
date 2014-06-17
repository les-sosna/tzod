#include "gc/VehicleState.h"
#include "core/PtrList.h"

#include <map>
#include <memory>
#include <string>

class AIController;
class GC_Player;
class GC_Object;
class World;

class AIManager
{
public:
	AIManager();
	~AIManager();
    void AssignAI(GC_Player *player, std::string profile);
    void FreeAI(GC_Player *player);
	
	typedef std::map<PtrList<GC_Object>::id_type, VehicleState> ControllerStateMap;
	ControllerStateMap ComputeAIState(World &world, float dt);
	
private:
	std::map<GC_Player *, std::pair<std::string, std::unique_ptr<AIController>>> _aiControllers;
};

