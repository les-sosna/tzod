#pragma once
#include <ui/ListBase.h>
#include <ui/Navigation.h>
#include <ui/Window.h>
#include <array>
#include <memory>
#include <vector>

class AppConfig;
class ShellConfig;
class LangCache;
class DMCampaign;
class WorldCache;
class MapPreview;
class World;
class WorldView;
namespace FS
{
	class FileSystem;
}
namespace UI
{
	class List;
	class StackLayout;
	class Text;
}

class SinglePlayer final
	: public UI::Window
	, private UI::NavigationSink
{
public:
	SinglePlayer(WorldView &worldView, FS::FileSystem &fs, AppConfig &appConfig, ShellConfig &conf, DMCampaign &dmCampaign, WorldCache &mapCache);

	std::function<void(std::shared_ptr<SinglePlayer>, int)> eventSelectMap;

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const UI::DataContext &dc, float scale, const UI::LayoutConstraints &layoutConstraints) const override;
	bool HasNavigationSink() const override { return true; }
	NavigationSink* GetNavigationSink() override { return this; }

private:
	int GetNextTier(UI::Navigate navigate) const;
	void UpdateTier();
	void OnOK(int index);

	WorldView &_worldView;
	FS::FileSystem &_fs;
	AppConfig &_appConfig;
	ShellConfig &_conf;
	DMCampaign &_dmCampaign;
	WorldCache &_worldCache;
	UI::ListDataSourceDefault _tiersSource;

	std::shared_ptr<UI::StackLayout> _content;
	std::shared_ptr<UI::StackLayout> _mapTiles;
	std::shared_ptr<UI::List> _tierSelector;

	// UI::NavigationSink
	bool CanNavigate(UI::Navigate navigate, const UI::LayoutContext &lc) const override;
	void OnNavigate(UI::Navigate navigate, UI::NavigationPhase phase, const UI::LayoutContext &lc) override;
};
