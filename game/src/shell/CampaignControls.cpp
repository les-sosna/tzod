#include "CampaignControls.h"
#include <ctx/Deathmatch.h>
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/Rating.h>
#include <ui/StackLayout.h>

using namespace UI::DataSourceAliases;

CampaignControls::CampaignControls(const Deathmatch &deathmatch, CampaignControlCommands commands)
	: _content(std::make_shared<UI::StackLayout>())
{
	_content->SetFlowDirection(UI::FlowDirection::Horizontal);
	_content->SetSpacing(20);
	AddFront(_content);

	auto replay = std::make_shared<UI::Button>();
	replay->Resize(200, 50);
	replay->SetFont("font_default");
	replay->SetText("Replay"_txt);
	replay->eventClick = std::move(commands.replayCurrent);
	_content->AddFront(replay);

	auto next = std::make_shared<UI::Button>();
	next->Resize(200, 50);
	next->SetFont("font_default");
	next->SetText("Next"_txt);
	next->eventClick = std::move(commands.playNext);
	_content->AddFront(next);
	_content->SetFocus(next.get());
}

UI::WindowLayout CampaignControls::GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	if (_content.get() == &child)
	{
		return UI::WindowLayout{ MakeRectWH(lc.GetPixelSize()), 1, true };
	}
	return UI::Window::GetChildLayout(texman, lc, dc, child);
}

vec2d CampaignControls::GetContentSize(TextureManager &texman, const UI::DataContext &dc, float scale, const UI::LayoutConstraints &layoutConstraints) const
{
	return _content->GetContentSize(texman, dc, scale, layoutConstraints);
}

std::shared_ptr<const UI::Window> CampaignControls::GetFocus(const std::shared_ptr<const Window>& owner) const
{
	return _content;
}

const UI::Window* CampaignControls::GetFocus() const
{
	return _content.get();
}
