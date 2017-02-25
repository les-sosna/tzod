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
#include <iomanip>

static const float c_tileSize = 200;
static const float c_tileSpacing = 16;

using namespace UI::DataSourceAliases;

namespace
{
	class TierProgressBinding : public UI::DataSource<unsigned int>
	{
	public:
		TierProgressBinding(AppConfig &appConfig, ShellConfig &conf, DMCampaign &dmCampaign)
			: _appConfig(appConfig)
			, _conf(conf)
			, _dmCampaign(dmCampaign)
		{}

		unsigned int GetValue(const UI::StateContext &sc) const override
		{
			int tier = GetCurrentTier(_conf, _dmCampaign);

			if (tier < (int)_appConfig.sp_tiersprogress.GetSize())
			{
				ConfVarArray &tierProgress = _appConfig.sp_tiersprogress.GetArray(tier);
				unsigned int index = sc.GetItemIndex();
				return index < tierProgress.GetSize() ? tierProgress.GetNum(index).GetInt() : 0;
			}
			else
			{
				return 0;
			}
		}

	private:
		AppConfig &_appConfig;
		ShellConfig &_conf;
		DMCampaign &_dmCampaign;
	};
}

SinglePlayer::SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs, AppConfig &appConfig, ShellConfig &conf, LangCache &lang, DMCampaign &dmCampaign)
	: UI::Dialog(manager, texman)
	, _appConfig(appConfig)
	, _conf(conf)
	, _lang(lang)
	, _dmCampaign(dmCampaign)
	, _content(std::make_shared<UI::StackLayout>(manager))
	, _tierTitle(std::make_shared<UI::Text>(manager, texman))
	, _prevTier(std::make_shared<UI::Button>(manager, texman))
	, _nextTier(std::make_shared<UI::Button>(manager, texman))
	, _mapTiles(std::make_shared<UI::List>(manager, texman, &_tilesSource))
	, _description(std::make_shared<UI::StackLayout>(manager))
	, _players(std::make_shared<UI::StackLayout>(manager))
{
	_tierTitle->SetFont(texman, "font_default");
	_content->AddFront(_tierTitle);

	auto mapTilesWithTierButtons = std::make_shared<UI::StackLayout>(manager);
	mapTilesWithTierButtons->SetFlowDirection(UI::FlowDirection::Horizontal);

	_prevTier->SetText("<<<"_txt);
	_prevTier->eventClick = std::bind(&SinglePlayer::OnPrevTier, this);
	_prevTier->SetFont(texman, "font_default");
	_prevTier->Resize(c_tileSize / 2, c_tileSize / 2);
	mapTilesWithTierButtons->AddFront(_prevTier);

	auto mp = std::make_shared<MapPreview>(manager, texman, fs, worldView, _mapCache);
	mp->Resize(c_tileSize, c_tileSize);
	mp->SetPadding(c_tileSpacing / 2);
	mp->SetMapName(std::make_shared<UI::ListDataSourceBinding>(0));
	mp->SetRating(std::make_shared<TierProgressBinding>(appConfig, conf, dmCampaign));

	_mapTiles->SetItemTemplate(mp);
	_mapTiles->SetFlowDirection(UI::FlowDirection::Horizontal);
	_mapTiles->eventChangeCurSel = std::bind(&SinglePlayer::OnSelectMap, this, std::ref(manager), std::ref(texman), std::placeholders::_1);
	mapTilesWithTierButtons->AddFront(_mapTiles);
	mapTilesWithTierButtons->SetFocus(_mapTiles);

	_nextTier->SetText(">>>"_txt);
	_nextTier->eventClick = std::bind(&SinglePlayer::OnNextTier, this);
	_nextTier->SetFont(texman, "font_default");
	_nextTier->Resize(c_tileSize / 2, c_tileSize / 2);
	mapTilesWithTierButtons->AddFront(_nextTier);

	_content->AddFront(mapTilesWithTierButtons);
	_content->SetFocus(mapTilesWithTierButtons);

	_description->SetSpacing(2);
	_content->AddFront(_description);

	_players->SetFlowDirection(UI::FlowDirection::Horizontal);
	_content->AddFront(_players);

	auto btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(ConfBind(lang.dmcampaign_ok));
//	btn->SetBackground(texman, nullptr, false);
	btn->SetFont(texman, "font_default");
	btn->Resize(200, 60);
	btn->eventClick = std::bind(&SinglePlayer::OnOK, this);
	_content->AddFront(btn);

	_content->SetSpacing(32);
	_content->SetAlign(UI::Align::CT);
	AddFront(_content);
	SetFocus(_content);

	SetDrawBackground(false);
	SetDrawBorder(false);
	UpdateTier();
}

void SinglePlayer::UpdateTier()
{
	int currentTier = GetCurrentTier(_conf, _dmCampaign);
	DMCampaignTier tierDesc(&_dmCampaign.tiers.GetTable(currentTier));

	_tierTitle->SetText(ConfBind(tierDesc.title));

	_prevTier->SetVisible(currentTier > 0);
	_nextTier->SetVisible(currentTier + 1 < (int)_dmCampaign.tiers.GetSize());

	_tilesSource.DeleteAllItems();

	for (size_t i = 0; i < tierDesc.maps.GetSize(); i++)
	{
		DMCampaignMapDesc mapDesc(&tierDesc.maps.GetTable(i));
		_tilesSource.AddItem(mapDesc.map_name.Get());
	}

	_mapTiles->SetCurSel(GetCurrentMap(_conf, _dmCampaign));
}

void SinglePlayer::OnPrevTier()
{
	if (_conf.sp_tier.GetInt() > 0)
	{
		_conf.sp_tier.SetInt(_conf.sp_tier.GetInt() - 1);
		_conf.sp_map.SetInt(0);
		UpdateTier();
	}
}

void SinglePlayer::OnNextTier()
{
	if (_conf.sp_tier.GetInt() + 1 < (int)_dmCampaign.tiers.GetSize())
	{
		_conf.sp_tier.SetInt(_conf.sp_tier.GetInt() + 1);
		_conf.sp_map.SetInt(0);
		UpdateTier();
	}
}

void SinglePlayer::OnOK()
{
	int index = _mapTiles->GetCurSel();
	if (-1 != index)
	{
		Close(_resultOK);
	}
}

void SinglePlayer::OnSelectMap(UI::LayoutManager &manager, TextureManager &texman, int index)
{
	_players->UnlinkAllChildren();
	_description->UnlinkAllChildren();

	if (-1 != index)
	{
		_conf.sp_map.SetInt(index);

		DMCampaignTier tierDesc(&_dmCampaign.tiers.GetTable(GetCurrentTier(_conf, _dmCampaign)));
		DMCampaignMapDesc mapDesc(&tierDesc.maps.GetTable(index));

		auto playerView = std::make_shared<PlayerView>(manager, texman);
		playerView->SetPlayerConfig(_appConfig.sp_playerinfo, texman);
		playerView->Resize(64, 64);
		_players->AddFront(playerView);

		auto versus = std::make_shared<UI::Text>(manager, texman);
		versus->SetFont(texman, "font_default");
		versus->SetText(ConfBind(_lang.dmcampaign_versus));
		_players->AddFront(versus);

		for (size_t i = 0; i < mapDesc.bot_names.GetSize(); ++i)
		{
			auto botView = std::make_shared<BotView>(manager, texman);
			botView->SetBotConfig(_dmCampaign.bots.GetTable(mapDesc.bot_names.GetStr(i).Get()), texman);
			botView->Resize(64, 64);
			_players->AddFront(botView);
		}

		{
			int seconds = (int)std::ceil(mapDesc.timelimit.GetFloat() * 60);

			std::ostringstream formatter;
			formatter << _lang.dmcampaign_timelimit.Get();
			formatter << (seconds / 60) << ":" << std::setfill('0') << std::setw(2) << (seconds % 60);

			auto timelimit = std::make_shared<UI::Text>(manager, texman);
			timelimit->SetText(std::make_shared<UI::StaticText>(formatter.str()));
			timelimit->SetFont(texman, "font_default");

			_description->AddFront(timelimit);
		}

		{
			auto fraglimit = std::make_shared<UI::Text>(manager, texman);

			std::ostringstream formatter;
			formatter << _lang.dmcampaign_fraglimit.Get() << mapDesc.fraglimit.GetInt();
			fraglimit->SetText(std::make_shared<UI::StaticText>(formatter.str()));
			fraglimit->SetFont(texman, "font_default");

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
