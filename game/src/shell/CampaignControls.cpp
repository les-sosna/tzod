#include "CampaignControls.h"
#include <ctx/Deathmatch.h>
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/Rating.h>
#include <ui/StackLayout.h>

using namespace UI::DataSourceAliases;

CampaignControls::CampaignControls(TextureManager &texman, const Deathmatch &deathmatch, CampaignControlCommands commands)
	: _content(std::make_shared<UI::StackLayout>())
{
	_content->SetFlowDirection(UI::FlowDirection::Horizontal);
	_content->SetSpacing(20);
	AddFront(_content);

	auto replay = std::make_shared<UI::Button>(texman);
	replay->Resize(200, 50);
	replay->SetFont(texman, "font_default");
	replay->SetText("Replay"_txt);
	replay->eventClick = std::move(commands.replayCurrent);
	_content->AddFront(replay);

	auto next = std::make_shared<UI::Button>(texman);
	next->Resize(200, 50);
	next->SetFont(texman, "font_default");
	next->SetText("Next"_txt);
	next->eventClick = std::move(commands.playNext);
	_content->AddFront(next);

}

FRECT CampaignControls::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	if (_content.get() == &child)
	{
		return MakeRectWH(lc.GetPixelSize());
	}
	return UI::Window::GetChildRect(texman, lc, dc, child);
}

vec2d CampaignControls::GetContentSize(TextureManager &texman, const UI::DataContext &dc, float scale) const
{
	return _content->GetContentSize(texman, dc, scale);
}
