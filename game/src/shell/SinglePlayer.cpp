#include "BotView.h"
#include "ConfigBinding.h"
#include "MapPreview.h"
#include "PlayerView.h"
#include "SinglePlayer.h"
#include "inc/shell/Config.h"
#include <MapFile.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <loc/Language.h>
#include <render/WorldView.h>
#include <video/DrawingContext.h>
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/List.h>
#include <ui/StackLayout.h>
#include <ui/StateContext.h>
#include <ui/Text.h>
#include <sstream>

static const float c_tileSize = 180;
static const float c_tileSpacing = 16;

using namespace UI::DataSourceAliases;

namespace
{
	class TierProgressBinding : public UI::DataSource<unsigned int>
	{
	public:
		TierProgressBinding(unsigned int tier, ConfVarArray &tiersProgress)
			: _tier(tier)
			, _tiersProgress(tiersProgress)
		{}

		unsigned int GetValue(const UI::StateContext &sc) const override
		{
			if (_tier < _tiersProgress.GetSize())
			{
				ConfVarArray &tierProgress = _tiersProgress.GetArray(_tier);
				unsigned int index = sc.GetItemIndex();
				return index < tierProgress.GetSize() ? tierProgress.GetNum(index).GetInt() : 0;
			}
			else
			{
				return 0;
			}
		}

	private:
		unsigned int _tier;
		ConfVarArray &_tiersProgress; // array of arrays
	};
}

SinglePlayer::SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs, AppConfig &appConfig, ShellConfig &conf, LangCache &lang, DMCampaign &dmCampaign)
	: UI::Dialog(manager, texman)
	, _conf(conf)
	, _lang(lang)
	, _dmCampaign(dmCampaign)
	, _content(std::make_shared<UI::StackLayout>(manager))
	, _tierTitle(std::make_shared<UI::Text>(manager, texman))
	, _tiles(std::make_shared<UI::List>(manager, texman, &_tilesSource))
	, _description(std::make_shared<UI::StackLayout>(manager))
	, _enemies(std::make_shared<UI::StackLayout>(manager))
{
	int currentTier = GetCurrentTier(conf, dmCampaign);
	DMCampaignTier tierDesc(&dmCampaign.tiers.GetTable(currentTier));

	_tierTitle->SetFont(texman, "font_default");
	_tierTitle->SetText(ConfBind(tierDesc.title));
	_content->AddFront(_tierTitle);

	for (size_t i = 0; i < tierDesc.maps.GetSize(); i++)
	{
		DMCampaignMapDesc mapDesc(&tierDesc.maps.GetTable(i));
		_tilesSource.AddItem(mapDesc.map_name.Get());
	}

	auto mp = std::make_shared<MapPreview>(manager, texman, fs, worldView, _mapCache);
	mp->Resize(c_tileSize, c_tileSize);
	mp->SetPadding(c_tileSpacing / 2);
	mp->SetMapName(std::make_shared<UI::ListDataSourceBinding>(0));
	mp->SetRating(std::make_shared<TierProgressBinding>(currentTier, appConfig.sp_tiersprogress));

	_tiles->SetItemTemplate(mp);
	_tiles->SetFlowDirection(UI::FlowDirection::Horizontal);
	_tiles->eventChangeCurSel = std::bind(&SinglePlayer::OnSelectMap, this, std::ref(manager), std::ref(texman), std::placeholders::_1);
	_content->AddFront(_tiles);
	_content->SetFocus(_tiles);

	auto descriptionTitle = std::make_shared<UI::Text>(manager, texman);
	descriptionTitle->SetFont(texman, "font_default");
	descriptionTitle->SetText(ConfBind(lang.dmcampaign_description));
	_content->AddFront(descriptionTitle);

	_description->SetSpacing(2);
	_content->AddFront(_description);

	auto enemiesTitle = std::make_shared<UI::Text>(manager, texman);
	enemiesTitle->SetFont(texman, "font_default");
	enemiesTitle->SetText(ConfBind(lang.dmcampaign_enemies));
	_content->AddFront(enemiesTitle);

	_enemies->SetFlowDirection(UI::FlowDirection::Horizontal);
	_content->AddFront(_enemies);

	auto playerTitle = std::make_shared<UI::Text>(manager, texman);
	playerTitle->SetFont(texman, "font_default");
	playerTitle->SetText(ConfBind(lang.dmcampaign_player));
	_content->AddFront(playerTitle);

	auto playerInfo = std::make_shared<UI::StackLayout>(manager);
	playerInfo->SetFlowDirection(UI::FlowDirection::Horizontal);
	_content->AddFront(playerInfo);

	auto playerView = std::make_shared<PlayerView>(manager, texman);
	playerView->SetPlayerConfig(appConfig.sp_playerinfo, texman);
	playerView->Resize(64, 64);
	playerInfo->AddFront(playerView);

	auto buttons = std::make_shared<UI::StackLayout>(manager);
	buttons->SetFlowDirection(UI::FlowDirection::Horizontal);
	buttons->SetSpacing(c_tileSpacing);
	_content->AddFront(buttons);

	auto btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(ConfBind(lang.dmcampaign_ok));
	btn->eventClick = std::bind(&SinglePlayer::OnOK, this);
	buttons->AddFront(btn);

	btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(ConfBind(lang.dmcampaign_cancel));
	btn->eventClick = std::bind(&SinglePlayer::OnCancel, this);
	buttons->AddFront(btn);

	_content->SetSpacing(c_tileSpacing);
	AddFront(_content);
	SetFocus(_content);

	_tiles->SetCurSel(GetCurrentMap(conf, dmCampaign));

	SetDrawBackground(false);
	SetDrawBorder(false);
}

void SinglePlayer::OnOK()
{
	int index = _tiles->GetCurSel();
	if (-1 != index)
	{
		Close(_resultOK);
	}
}

void SinglePlayer::OnCancel()
{
	Close(_resultCancel);
}

void SinglePlayer::OnSelectMap(UI::LayoutManager &manager, TextureManager &texman, int index)
{
	_enemies->UnlinkAllChildren();
	_description->UnlinkAllChildren();

	if (-1 != index)
	{
		_conf.sp_map.SetInt(index);

		DMCampaignTier tierDesc(&_dmCampaign.tiers.GetTable(GetCurrentTier(_conf, _dmCampaign)));
		DMCampaignMapDesc mapDesc(&tierDesc.maps.GetTable(index));

		for (size_t i = 0; i < mapDesc.bot_names.GetSize(); ++i)
		{
			auto botView = std::make_shared<BotView>(manager, texman);
			botView->SetBotConfig(_dmCampaign.bots.GetTable(mapDesc.bot_names.GetStr(i).Get()), texman);
			botView->Resize(64, 64);
			_enemies->AddFront(botView);
		}

		{
			auto timelimit = std::make_shared<UI::Text>(manager, texman);

			std::ostringstream formatter;
			formatter << _lang.dmcampaign_timelimit.Get() << mapDesc.timelimit.GetFloat();
			timelimit->SetText(std::make_shared<UI::StaticText>(formatter.str()));

			_description->AddFront(timelimit);
		}

		{
			auto fraglimit = std::make_shared<UI::Text>(manager, texman);

			std::ostringstream formatter;
			formatter << _lang.dmcampaign_fraglimit.Get() << mapDesc.fraglimit.GetInt();
			fraglimit->SetText(std::make_shared<UI::StaticText>(formatter.str()));

			_description->AddFront(fraglimit);
		}
	}
}

FRECT SinglePlayer::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const
{
	if (_content.get() == &child)
	{
		vec2d pxMargins = UI::ToPx(vec2d{ c_tileSpacing, c_tileSpacing }, lc);
		return MakeRectRB(pxMargins, lc.GetPixelSize() - pxMargins);
	}

	return UI::Dialog::GetChildRect(texman, lc, sc, child);
}

vec2d SinglePlayer::GetContentSize(TextureManager &texman, const UI::StateContext &sc, float scale) const
{
	vec2d pxMargins = UI::ToPx(vec2d{ c_tileSpacing, c_tileSpacing }, scale);
	return _content->GetContentSize(texman, sc, scale) + pxMargins * 2;
}
