#pragma once
#include "Controller.h"
#include <gc/WorldEvents.h>
#include <vector>

class ConfCache;

namespace UI
{
	class ConsoleBuffer;
}

class InputManager
{
public:
	InputManager(ConfCache &conf, UI::ConsoleBuffer &logger);
	~InputManager();
	Controller* GetController(unsigned int index);

private:
	ConfCache &_conf;
	UI::ConsoleBuffer &_logger;
	std::vector<Controller> _controllers;
	void OnProfilesChange();

	InputManager(const InputManager&) = delete;
	void operator=(const InputManager&) = delete;
};

