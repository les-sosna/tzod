#pragma once
#include <functional>

struct CampaignControlCommands
{
	std::function<void()> playNext;
	std::function<void()> replayCurrent;
	std::function<void()> quitCurrent;
	std::function<void()> systemSettings;
};
