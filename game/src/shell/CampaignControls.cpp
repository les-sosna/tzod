#include "CampaignControls.h"
#include <ui/Button.h>
#include <ui/Rating.h>

CampaignControls::CampaignControls(UI::LayoutManager &manager, TextureManager &texman)
	: UI::Window(manager)
{
	auto replay = std::make_shared<UI::Button>(manager, texman);
	auto next = std::make_shared<UI::Button>(manager, texman);

	auto rating = std::make_shared<UI::Rating>(manager, texman);
}

