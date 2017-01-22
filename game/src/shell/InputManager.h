#pragma once
#include "Controller.h"
#include <gc/WorldEvents.h>
#include <vector>

class ShellConfig;

namespace UI
{
	class ConsoleBuffer;
}

class InputManager
{
public:
	InputManager(ShellConfig &conf, UI::ConsoleBuffer &logger);
	~InputManager();
	Controller* GetController(unsigned int index);
	const Controller* GetController(unsigned int index) const;

    void Step(float dt);

private:
	ShellConfig &_conf;
	UI::ConsoleBuffer &_logger;
	std::vector<Controller> _controllers;
	void OnProfilesChange();

	InputManager(const InputManager&) = delete;
	void operator=(const InputManager&) = delete;
};

