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
#include <video/RenderContext.h>
#include <ui/Button.h>
#include <ui/DataContext.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/List.h>
#include <ui/Rectangle.h>
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
	int GetMapRating(AppConfig &appConfig, int tierIndex, size_t mapIndex)
	{
		if (tierIndex < (int)appConfig.sp_tiersprogress.GetSize())
		{
			ConfVarArray &tierProgress = appConfig.sp_tiersprogress.GetArray(tierIndex);
			return mapIndex < tierProgress.GetSize() ? tierProgress.GetNum(mapIndex).GetInt() : 0;
		}
		else
		{
			return 0;
		}
	}

	class TierProgressListBinding : public UI::RenderData<unsigned int>
	{
	public:
		TierProgressListBinding(AppConfig &appConfig, ShellConfig &conf, DMCampaign &dmCampaign)
			: _appConfig(appConfig)
			, _conf(conf)
			, _dmCampaign(dmCampaign)
		{}

		unsigned int GetValue(const UI::DataContext &dc, const UI::StateContext &sc) const override
		{
			return GetMapRating(_appConfig, GetCurrentTier(_conf, _dmCampaign), dc.GetItemIndex());
		}

	private:
		AppConfig &_appConfig;
		ShellConfig &_conf;
		DMCampaign &_dmCampaign;
	};

	class TierProgressIndexBinding : public UI::RenderData<unsigned int>
	{
	public:
		TierProgressIndexBinding(AppConfig &appConfig, ShellConfig &conf, DMCampaign &dmCampaign, size_t mapIndex)
			: _appConfig(appConfig)
			, _conf(conf)
			, _dmCampaign(dmCampaign)
			, _mapIndex(mapIndex)
		{}

		unsigned int GetValue(const UI::DataContext &dc, const UI::StateContext &sc) const override
		{
			return GetMapRating(_appConfig, GetCurrentTier(_conf, _dmCampaign), _mapIndex);
		}

	private:
		AppConfig &_appConfig;
		ShellConfig &_conf;
		DMCampaign &_dmCampaign;
		size_t _mapIndex;
	};

	const auto c_tierBoxFrame = std::make_shared<UI::StateBinding<unsigned int>>(1, // default
		UI::StateBinding<unsigned int>::MapType{ { "Normal", 0 } });

	class TierBox : public UI::Rectangle
	{
	public:
		TierBox()
		{
			Resize(48, 48);

			auto center = std::make_shared<UI::Rectangle>();
			center->SetTexture("ui/list");
			center->SetFrame(c_tierBoxFrame);
			AddFront(center);
		}

		FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override
		{
			return MakeRectWH(lc.GetPixelSize() / 4, lc.GetPixelSize() / 2);
		}
	};
}

SinglePlayer::SinglePlayer(WorldView &worldView, FS::FileSystem &fs, AppConfig &appConfig, ShellConfig &conf, LangCache &lang, DMCampaign &dmCampaign, MapCache &mapCache)
	: _worldView(worldView)
	, _fs(fs)
	, _appConfig(appConfig)
	, _conf(conf)
	, _lang(lang)
	, _dmCampaign(dmCampaign)
	, _mapCache(mapCache)
	, _content(std::make_shared<UI::StackLayout>())
	, _prevTier(std::make_shared<UI::Button>())
	, _mapTiles(std::make_shared<UI::StackLayout>())
	, _nextTier(std::make_shared<UI::Button>())
	, _tierSelector(std::make_shared<UI::List>(&_tiersSource))
{
	auto mapTilesWithTierButtons = std::make_shared<UI::StackLayout>();
	mapTilesWithTierButtons->SetFlowDirection(UI::FlowDirection::Horizontal);

	_prevTier->SetText("<<<"_txt);
	_prevTier->eventClick = std::bind(&SinglePlayer::OnPrevTier, this);
	_prevTier->SetFont("font_default");
	_prevTier->Resize(c_tileSize / 2, c_tileSize / 2);
	mapTilesWithTierButtons->AddFront(_prevTier);

	_mapTiles->SetFlowDirection(UI::FlowDirection::Horizontal);
	mapTilesWithTierButtons->AddFront(_mapTiles);
	mapTilesWithTierButtons->SetFocus(_mapTiles);

	mapTilesWithTierButtons->AddFront(_nextTier);

	_nextTier->SetText(">>>"_txt);
	_nextTier->eventClick = std::bind(&SinglePlayer::OnNextTier, this);
	_nextTier->SetFont("font_default");
	_nextTier->Resize(c_tileSize / 2, c_tileSize / 2);

	_content->AddFront(mapTilesWithTierButtons);
	_content->SetFocus(mapTilesWithTierButtons);

	for (size_t i = 0; i < _dmCampaign.tiers.GetSize(); i++)
	{
		_tiersSource.AddItem("");
	}

	_tierSelector->SetItemTemplate(std::make_shared<TierBox>());
	_tierSelector->SetFlowDirection(UI::FlowDirection::Horizontal);
	_tierSelector->eventChangeCurSel = [=](int index)
	{
		if (index != -1)
		{
			_conf.sp_tier.SetInt(index);
			_conf.sp_map.SetInt(0);
			UpdateTier();
		}
	};
	_content->AddFront(_tierSelector);

	_content->SetSpacing(64);
	_content->SetAlign(UI::Align::CT);
	AddFront(_content);
	SetFocus(_content);

	UpdateTier();
}

static int GetTierRating(const DMCampaign &dmCampaign, AppConfig &appConfig, int tier)
{
	int tierRating = INT_MAX;
	DMCampaignTier tierDesc(&dmCampaign.tiers.GetTable(tier));
	for (size_t mapIndex = 0; mapIndex < tierDesc.maps.GetSize(); mapIndex++)
	{
		tierRating = std::min(tierRating, GetMapRating(appConfig, tier, mapIndex));
	}
	return tierRating;
}

void SinglePlayer::UpdateTier()
{
	int currentTier = GetCurrentTier(_conf, _dmCampaign);
	DMCampaignTier tierDesc(&_dmCampaign.tiers.GetTable(currentTier));

	_prevTier->SetVisible(currentTier > 0);
	_nextTier->SetVisible(currentTier + 1 < (int)_dmCampaign.tiers.GetSize());
	_tierSelector->SetCurSel(currentTier);

	_mapTiles->UnlinkAllChildren();

	bool locked = currentTier> 0 && GetTierRating(_dmCampaign, _appConfig, currentTier - 1) == 0;

	for (size_t mapIndex = 0; mapIndex < tierDesc.maps.GetSize(); mapIndex++)
	{
		DMCampaignMapDesc mapDesc(&tierDesc.maps.GetTable(mapIndex));

		auto mapPreview = std::make_shared<MapPreview>(_fs, _worldView, _mapCache);
		mapPreview->Resize(c_tileSize, c_tileSize);
		mapPreview->SetPadding(c_tileSpacing / 2);
		mapPreview->SetMapName(std::make_shared<UI::StaticText>(mapDesc.map_name.Get()));
		mapPreview->SetRating(std::make_shared<TierProgressIndexBinding>(_appConfig, _conf, _dmCampaign, mapIndex));

		auto mpButton = std::make_shared<UI::ButtonBase>();
		mpButton->AddFront(mapPreview);
		mpButton->Resize(c_tileSize, c_tileSize);
		mpButton->eventClick = std::bind(&SinglePlayer::OnOK, this, (int)mapIndex);
		if (locked)
		{
			mpButton->SetEnabled(UI::StaticValue<bool>::False());
		}

		_mapTiles->AddFront(mpButton);
	}
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

void SinglePlayer::OnOK(int index)
{
	if (-1 != index && eventSelectMap)
	{
		eventSelectMap(std::static_pointer_cast<SinglePlayer>(shared_from_this()), index);
	}
}

FRECT SinglePlayer::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	if (_content.get() == &child)
	{
		vec2d pxMargins = UI::ToPx(vec2d{ c_tileSpacing, c_tileSpacing }, lc);
		return MakeRectRB(pxMargins, lc.GetPixelSize() - pxMargins);
	}

	return UI::Window::GetChildRect(texman, lc, dc, child);
}

vec2d SinglePlayer::GetContentSize(TextureManager &texman, const UI::DataContext &dc, float scale) const
{
	vec2d pxMargins = UI::ToPx(vec2d{ c_tileSpacing, c_tileSpacing }, scale);
	return _content->GetContentSize(texman, dc, scale) + pxMargins * 2;
}
