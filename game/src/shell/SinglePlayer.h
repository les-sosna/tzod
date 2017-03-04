#pragma once
#include "MapCache.h"
#include <ui/ListBase.h>
#include <ui/Window.h>
#include <array>
#include <memory>
#include <vector>

class AppConfig;
class ShellConfig;
class LangCache;
class DMCampaign;
class MapPreview;
class World;
class WorldView;
namespace FS
{
	class FileSystem;
}
namespace UI
{
	class Button;
	class List;
	class StackLayout;
	class Text;
}

class SinglePlayer : public UI::Window
{
public:
	SinglePlayer(WorldView &worldView, FS::FileSystem &fs, AppConfig &appConfig, ShellConfig &conf, LangCache &lang, DMCampaign &dmCampaign);

	std::function<void(std::shared_ptr<SinglePlayer>, int)> eventSelectMap;

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const UI::DataContext &dc, float scale) const override;

private:
	void UpdateTier();
	void OnPrevTier();
	void OnNextTier();
	void OnOK(int index);

	WorldView &_worldView;
	FS::FileSystem &_fs;
	AppConfig &_appConfig;
	ShellConfig &_conf;
	LangCache &_lang;
	DMCampaign &_dmCampaign;
	MapCache _mapCache;
	UI::ListDataSourceDefault _tiersSource;

	std::shared_ptr<UI::StackLayout> _content;
	std::shared_ptr<UI::Button> _prevTier;
	std::shared_ptr<UI::StackLayout> _mapTiles;
	std::shared_ptr<UI::Button> _nextTier;
};
