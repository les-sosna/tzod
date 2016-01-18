#include "Controller.h"
#include "InputManager.h"
#include "inc/shell/Config.h"
#include <ui/ConsoleBuffer.h>

InputManager::InputManager(ConfCache &conf, UI::ConsoleBuffer &logger)
	: _conf(conf)
	, _logger(logger)
{
	_conf.dm_profiles.eventChange = std::bind(&InputManager::OnProfilesChange, this);
	OnProfilesChange();
}

InputManager::~InputManager()
{
	_conf.dm_profiles.eventChange = nullptr;
}

Controller* InputManager::GetController(unsigned int index)
{
	return index < _controllers.size() ? &_controllers[index] : nullptr;
}

void InputManager::Step(float dt)
{
    for (auto &controller: _controllers)
    {
        controller.Step(dt);
    }
}

void InputManager::OnProfilesChange()
{
	_controllers.clear();

	for (auto &key: _conf.dm_profiles.GetKeys())
	{
		_controllers.emplace_back();

		ConfVar *p = _conf.dm_profiles.Find(key.c_str());
		if (p && ConfVar::typeTable == p->GetType())
		{
			ConfControllerProfile t(&p->AsTable());
			_controllers.back().SetProfile(t);
		}
		else
		{
			_logger.Printf(1, "controller profile '%s' not found", key.c_str());
		}
	}
}
