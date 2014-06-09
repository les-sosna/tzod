// InputManager.h

#pragma once

#include <map>
#include <memory>
#include <string>

class GC_Player;
class Controller;
class World;

class InputManager
{
public:
	InputManager();
    ~InputManager();
    
    void AssignController(GC_Player *player, std::string profile);
    void FreeController(GC_Player *player);
	Controller& GetController(GC_Player *player) const;

private:
	std::map<GC_Player *, std::pair<std::string, std::unique_ptr<Controller>>> _controllers;
	void OnProfilesChange();
};

// end of file
