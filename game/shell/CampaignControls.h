#pragma once
#include "CampaignControlCommands.h"
#include <ui/Window.h>

namespace UI
{
	class StackLayout;
}

class Deathmatch;

class CampaignControls : public UI::WindowContainer
{
public:
	CampaignControls(const Deathmatch &deathmatch, CampaignControlCommands commands);

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const UI::DataContext &dc, float scale, const UI::LayoutConstraints &layoutConstraints) const override;
	std::shared_ptr<const UI::Window> GetFocus(const std::shared_ptr<const Window>& owner) const override;
	const UI::Window* GetFocus() const override;

private:
	std::shared_ptr<UI::StackLayout> _content;
};
