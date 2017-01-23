#pragma once
#include "CampaignControlCommands.h"
#include <ui/Window.h>

namespace UI
{
	class Rating;
	class StackLayout;
}

class Deathmatch;

class CampaignControls : public UI::Window
{
public:
	CampaignControls(UI::LayoutManager &manager, TextureManager &texman, const Deathmatch &deathmatch, CampaignControlCommands commands);

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const UI::StateContext &sc, float scale) const override;

private:
	std::shared_ptr<UI::StackLayout> _content;
	std::shared_ptr<UI::Rating> _rating;
};
