// InputManager.h

#pragma once

#include "gc/WorldEvents.h"

#include <map>
#include <memory>
#include <string>

class GC_Player;
class Controller;
class World;

class InputManager
	: private ObjectListener
{
public:
	InputManager(World &world);
    ~InputManager();
    
    void AssignController(GC_Player *player, std::string profile);
	Controller* GetController(GC_Player *player) const;

private:
	std::map<GC_Player *, std::pair<std::string, std::unique_ptr<Controller>>> _controllers;
	void OnProfilesChange();
	World &_world;
	
	// ObjectListener
	virtual void OnCreate(GC_Object *obj) override;
	virtual void OnKill(GC_Object *obj) override;
};

// end of file
