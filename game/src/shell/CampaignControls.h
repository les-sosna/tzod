#pragma once
#include "CampaignControlCommands.h"
#include <ui/Window.h>

namespace UI
{
	class StackLayout;
}

class Deathmatch;

class CampaignControls : public UI::Window
{
public:
	CampaignControls(TextureManager &texman, const Deathmatch &deathmatch, CampaignControlCommands commands);

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const UI::DataContext &dc, float scale) const override;

private:
	std::shared_ptr<UI::StackLayout> _content;
};
