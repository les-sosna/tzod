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
	: private ObjectListener<GC_Player>
	, private ObjectListener<GC_Object>
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
	
	// ObjectListener<GC_Player>
	virtual void OnRespawn(GC_Player &, GC_Vehicle &) override {}
	virtual void OnDie(GC_Player &) override {}

	// ObjectListener<GC_Object>
	virtual void OnCreate(GC_Object &obj) override {}
	virtual void OnKill(GC_Object &obj) override;
};

// end of file
