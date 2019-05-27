#include "BotView.h"
#include "MapPreview.h"
#include "PlayerView.h"
#include "SinglePlayer.h"
#include "inc/shell/Config.h"
#include <MapFile.h>
#include <cbind/ConfigBinding.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <render/WorldView.h>
#include <video/RenderContext.h>
#include <ui/Button.h>
#include <ui/DataContext.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/List.h>
#include <ui/Rating.h>
#include <ui/Rectangle.h>
#include <ui/StackLayout.h>
#include <ui/StateContext.h>
#include <ui/Text.h>
#include <climits>
#include <sstream>
#include <iomanip>

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

		// UI::RenderData<unsigned int>
		unsigned int GetRenderValue(const UI::DataContext &dc, const UI::StateContext &sc) const override
		{
			return GetMapRating(_appConfig, GetCurrentTier(_conf, _dmCampaign), dc.GetItemIndex());
		}

	private:
		AppConfig &_appConfig;
		ShellConfig &_conf;
		DMCampaign &_dmCampaign;
	};

	class TierProgressIndexBinding final
		: public UI::RenderData<unsigned int>
	{
	public:
		TierProgressIndexBinding(AppConfig &appConfig, ShellConfig &conf, DMCampaign &dmCampaign, size_t mapIndex)
			: _appConfig(appConfig)
			, _conf(conf)
			, _dmCampaign(dmCampaign)
			, _mapIndex(mapIndex)
		{}

		// UI::RenderData<unsigned int>
		unsigned int GetRenderValue(const UI::DataContext &dc, const UI::StateContext &sc) const override
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

	class TierBox final
		: public UI::Rectangle
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

	class DifficultySelector final
		: public UI::ButtonBase
	{
	public:
		DifficultySelector(ConfVarNumber& confValue)
			: _confValue(confValue)
		{
			Resize(144, 48);

			auto background = std::make_shared<UI::Rectangle>();
			background->SetTexture("ui/button");
			background->SetFrame(std::make_shared<UI::StateBinding<unsigned int>>(0, // default
				UI::StateBinding<unsigned int>::MapType{ { "Hover", 1 }, { "Pushed", 1 } }));
			AddFront(background);

			auto rating = std::make_shared<UI::Rating>();
			rating->SetRating(std::make_shared<DifficultyConfigBinding>(confValue));
			AddFront(rating);

			auto proxy = std::make_shared<NavigationProxy>(confValue);
			AddFront(proxy);
			SetFocus(proxy);
		}

		// Window
		FRECT GetChildRect(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override
		{
			return MakeRectWH(lc.GetPixelSize());
		}

	private:
		// ButtonBase
		void OnClick() override
		{
			_confValue.SetInt((_confValue.GetInt() + 1) % 3);
		}

		class DifficultyConfigBinding final
			: public UI::RenderData<unsigned int>
		{
		public:
			DifficultyConfigBinding(const ConfVarNumber& confValue)
				: _confValue(confValue)
			{}

			// UI::RenderData<unsigned int>
			unsigned int GetRenderValue(const UI::DataContext& dc, const UI::StateContext& sc) const override
			{
				return _confValue.GetInt() + 1;
			}

		private:
			const ConfVarNumber& _confValue;
		};

		class NavigationProxy final
			: public UI::Window
			, private UI::NavigationSink
		{
		public:
			explicit NavigationProxy(ConfVarNumber& confValue)
				: _confValue(confValue)
			{}
			// UI::Window
			bool HasNavigationSink() const override { return true; }
			NavigationSink* GetNavigationSink() override { return this; }

		private:
			// UI::NavigationSink
			bool CanNavigate(UI::Navigate navigate, const UI::LayoutContext& lc) const override
			{
				switch (navigate)
				{
				case UI::Navigate::Left:
				case UI::Navigate::Right:
					return true;
				default:
					return false;
				}
			}
			void OnNavigate(UI::Navigate navigate, UI::NavigationPhase phase, const UI::LayoutContext& lc) override
			{
				if (phase == UI::NavigationPhase::Started)
				{
					switch (navigate)
					{
						case UI::Navigate::Left:
							_confValue.SetInt(std::max(0, _confValue.GetInt() - 1));
							break;
						case UI::Navigate::Right:
							_confValue.SetInt(std::min(2, _confValue.GetInt() + 1));
							break;
						default:
							break;
					}
				}
			}

			ConfVarNumber& _confValue;
		};

		ConfVarNumber& _confValue;
	};
}

SinglePlayer::SinglePlayer(WorldView &worldView, FS::FileSystem &fs, AppConfig &appConfig, ShellConfig &conf, DMCampaign &dmCampaign, WorldCache &mapCache)
	: _worldView(worldView)
	, _fs(fs)
	, _appConfig(appConfig)
	, _conf(conf)
	, _dmCampaign(dmCampaign)
	, _worldCache(mapCache)
	, _content(std::make_shared<UI::StackLayout>())
	, _mapTiles(std::make_shared<UI::StackLayout>())
	, _tierSelector(std::make_shared<UI::List>(&_tiersSource))
{
	auto difficultySelector = std::make_shared<DifficultySelector>(appConfig.sp_difficulty);
	_content->AddFront(difficultySelector);

	_mapTiles->SetFlowDirection(UI::FlowDirection::Horizontal);
	_content->AddFront(_mapTiles);
	_content->SetFocus(_mapTiles);

	for (size_t i = 0; i < _dmCampaign.tiers.GetSize(); i++)
	{
		_tiersSource.AddItem("");
	}

	_tierSelector->SetItemTemplate(std::make_shared<TierBox>());
	_tierSelector->SetFlowDirection(UI::FlowDirection::Horizontal);
	_tierSelector->SetEnableNavigation(false);
	_tierSelector->SetCurSel(GetCurrentTier(_conf, _dmCampaign));
	_tierSelector->eventChangeCurSel = [=](int index)
	{
		if (index != -1)
		{
			int prevTier = GetCurrentTier(_conf, _dmCampaign);
			_conf.sp_tier.SetInt(index);
			if (index > prevTier)
				_conf.sp_map.SetInt(0);
			else
				_conf.sp_map.SetInt(GetCurrentTierMapCount(_conf, _dmCampaign) - 1);
			UpdateTier();
		}
	};
	_content->AddFront(_tierSelector);

	_content->SetSpacing(48);
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

	_mapTiles->UnlinkAllChildren();

	bool locked = currentTier> 0 && GetTierRating(_dmCampaign, _appConfig, currentTier - 1) == 0;
	int currentMap = GetCurrentMap(_conf, _dmCampaign);

	for (size_t mapIndex = 0; mapIndex < tierDesc.maps.GetSize(); mapIndex++)
	{
		DMCampaignMapDesc mapDesc(&tierDesc.maps.GetTable(mapIndex));

		auto mapPreview = std::make_shared<MapPreview>(_fs, _worldView, _worldCache);
		mapPreview->Resize(_conf.ui_tile_size.GetFloat(), _conf.ui_tile_size.GetFloat());
		mapPreview->SetPadding(_conf.ui_tile_spacing.GetFloat() / 2);
		mapPreview->SetMapName(std::make_shared<UI::StaticText>(mapDesc.map_name.Get()));
		mapPreview->SetRating(std::make_shared<TierProgressIndexBinding>(_appConfig, _conf, _dmCampaign, mapIndex));
		mapPreview->SetLocked(locked);

		auto mpButton = std::make_shared<UI::ButtonBase>();
		mpButton->AddFront(mapPreview);
		mpButton->Resize(_conf.ui_tile_size.GetFloat(), _conf.ui_tile_size.GetFloat());
		if (!locked)
		{
			mpButton->eventClick = std::bind(&SinglePlayer::OnOK, this, (int)mapIndex);
		}

		_mapTiles->AddFront(mpButton);

		if (mapIndex == currentMap)
		{
			_mapTiles->SetFocus(mpButton);
		}
	}
}

int SinglePlayer::GetNextTier(UI::Navigate navigate) const
{
	int maxTier = (int)_dmCampaign.tiers.GetSize() - 1;
	switch (navigate)
	{
	case UI::Navigate::Left:
		return std::min(maxTier, std::max(0, _conf.sp_tier.GetInt() - 1));
	case UI::Navigate::Right:
		return std::min(maxTier, std::max(0, _conf.sp_tier.GetInt() + 1));
	default:
		return _conf.sp_tier.GetInt();
	}
}

void SinglePlayer::OnOK(int index)
{
	if (eventSelectMap)
	{
		eventSelectMap(index);
	}
}

FRECT SinglePlayer::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	if (_content.get() == &child)
	{
		vec2d pxMargins = UI::ToPx(vec2d{ _conf.ui_tile_spacing.GetFloat(), _conf.ui_tile_spacing.GetFloat() }, lc);
		return MakeRectRB(pxMargins, lc.GetPixelSize() - pxMargins);
	}

	return UI::Window::GetChildRect(texman, lc, dc, child);
}

vec2d SinglePlayer::GetContentSize(TextureManager &texman, const UI::DataContext &dc, float scale, const UI::LayoutConstraints &layoutConstraints) const
{
	vec2d pxMargins = UI::ToPx(vec2d{ _conf.ui_tile_spacing.GetFloat(), _conf.ui_tile_spacing.GetFloat() }, scale);
	return _content->GetContentSize(texman, dc, scale, layoutConstraints) + pxMargins * 2;
}

bool SinglePlayer::CanNavigate(UI::Navigate navigate, const UI::LayoutContext &lc) const
{
	return GetNextTier(navigate) != GetCurrentTier(_conf, _dmCampaign);
}

void SinglePlayer::OnNavigate(UI::Navigate navigate, UI::NavigationPhase phase, const UI::LayoutContext &lc)
{
	if (UI::NavigationPhase::Started == phase)
	{
		_tierSelector->SetCurSel(GetNextTier(navigate));
	}
}
