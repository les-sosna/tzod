#pragma once
#include "Controller.h"
#include <gc/WorldEvents.h>
#include <vector>

class InputManager
{
public:
	InputManager();
	~InputManager();
	Controller* GetController(unsigned int index);

private:
	std::vector<Controller> _controllers;
	void OnProfilesChange();

	InputManager(const InputManager&) = delete;
	void operator=(const InputManager&) = delete;
};

