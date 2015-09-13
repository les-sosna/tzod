#include "InputManager.h"
#include "Controller.h"
#include "Config.h"

InputManager::InputManager()
{
	g_conf.dm_profiles.eventChange = std::bind(&InputManager::OnProfilesChange, this);
	OnProfilesChange();
}

InputManager::~InputManager()
{
	g_conf.dm_profiles.eventChange = nullptr;
}

Controller* InputManager::GetController(unsigned int index)
{
	return index < _controllers.size() ? &_controllers[index] : nullptr;
}

void InputManager::OnProfilesChange()
{
	_controllers.clear();

	for (auto &key: g_conf.dm_profiles.GetKeys())
	{
		_controllers.emplace_back();
		_controllers.back().SetProfile(key.c_str());
	}
}
