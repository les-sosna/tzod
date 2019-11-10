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
	: public UI::WindowContainer
	, private UI::NavigationSink
{
public:
	SinglePlayer(WorldView &worldView, FS::FileSystem &fs, AppConfig &appConfig, ShellConfig &conf, DMCampaign &dmCampaign, WorldCache &mapCache, const LangCache &lang);

	std::function<void(int)> eventSelectMap;

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const UI::DataContext &dc, float scale, const UI::LayoutConstraints &layoutConstraints) const override;
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
	bool CanNavigate(TextureManager& texman, const UI::InputContext& ic, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate) const override;
	void OnNavigate(TextureManager& texman, const UI::InputContext& ic, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate, UI::NavigationPhase phase) override;
};
