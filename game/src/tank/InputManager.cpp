// InputManager.cpp

#include "InputManager.h"
#include "config/Config.h"

InputManager::InputManager()
{
    g_conf.dm_profiles.eventChange = std::bind(&InputManager::OnProfilesChange, this);
    OnProfilesChange();
}

InputManager::~InputManager()
{
    g_conf.dm_profiles.eventChange = nullptr;
}

void InputManager::ReadControllerState(const char *profile, const GC_Vehicle *vehicle, VehicleState &vs)
{
    auto it = _controllers.find(profile);
    if( vehicle && _controllers.end() != it )
    {
        it->second.ReadControllerState(vehicle, vs);
    }
}

void InputManager::OnProfilesChange()
{
    _controllers.clear();

    ConfVarTable::KeyListType keys;
    g_conf.dm_profiles.GetKeyList(keys);
    for( auto it = keys.begin(); it != keys.end(); ++it )
    {
        _controllers[*it].SetProfile(it->c_str());
    }
}

// end of file
