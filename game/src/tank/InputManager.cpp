// InputManager.cpp

#include "InputManager.h"
#include "Controller.h"
#include "config/Config.h"
#include "gc/Player.h"
#include "gc/Vehicle.h"

InputManager::InputManager()
{
    g_conf.dm_profiles.eventChange = std::bind(&InputManager::OnProfilesChange, this);
    OnProfilesChange();
}

InputManager::~InputManager()
{
    g_conf.dm_profiles.eventChange = nullptr;
    assert(_controllers.empty());
}

Controller& InputManager::GetController(GC_Player *player) const
{
	assert(_controllers.count(player));
	return *_controllers.find(player)->second.second;
}

void InputManager::AssignController(GC_Player *player, std::string profile)
{
    std::unique_ptr<Controller> ctrl(new Controller());
    ctrl->SetProfile(profile.c_str());
    _controllers[player] = std::make_pair(std::move(profile), std::move(ctrl));
}

void InputManager::FreeController(GC_Player *player)
{
    _controllers.erase(player);
}

void InputManager::OnProfilesChange()
{
    for (auto &pcpair: _controllers)
    {
        pcpair.second.second->SetProfile(pcpair.second.first.c_str());
    }
}

// end of file
