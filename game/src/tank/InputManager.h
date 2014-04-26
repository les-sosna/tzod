// InputManager.h

#pragma once

#include <map>
#include <memory>
#include <string>

class GC_Player;
class Controller;

class InputManager
{
public:
	InputManager();
    ~InputManager();
    
    void AssignController(GC_Player *player, std::unique_ptr<Controller> &&ctrl);
    void FreeController(GC_Player *player);

    void ReadControllerState();

private:
	std::map<GC_Player *, std::unique_ptr<Controller>> _controllers;
	void OnProfilesChange();
};

// end of file
