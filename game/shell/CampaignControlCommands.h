#pragma once
#include <functional>

struct CampaignControlCommands
{
	std::function<void()> playNext;
	std::function<void()> replayCurrent;
};
