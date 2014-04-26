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

void InputManager::ReadControllerState()
{
    for (auto &pcpair: _controllers)
    {
        if( GC_Vehicle *vehicle = pcpair.first->GetVehicle() )
        {
            VehicleState vs;
            pcpair.second->ReadControllerState(vehicle, vs);
            vehicle->SetControllerState(vs);
        }
    }
}

void InputManager::AssignController(GC_Player *player, std::unique_ptr<Controller> &&ctrl)
{
    _controllers[player] = std::move(ctrl);
}

void InputManager::FreeController(GC_Player *player)
{
    _controllers.erase(player);
}

void InputManager::OnProfilesChange()
{
    _controllers.clear();

    ConfVarTable::KeyListType keys;
    g_conf.dm_profiles.GetKeyList(keys);
    for( auto it = keys.begin(); it != keys.end(); ++it )
    {
//        _controllers[*it].SetProfile(it->c_str());
    }
}

// end of file
