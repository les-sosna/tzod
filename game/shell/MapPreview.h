#pragma once
#include <ui/Texture.h>
#include <ui/Window.h>
#include <memory>
#include <string>
class MapCollection;
class WorldView;

namespace FS
{
	class FileSystem;
}
namespace UI
{
	class Rating;
	template<class T> struct RenderData;
}

class MapPreview final
	: public UI::WindowContainer
{
public:
	MapPreview(FS::FileSystem &fs, WorldView &worldView, MapCollection &mapCollection);

	void SetMapName(std::shared_ptr<UI::RenderData<std::string_view>> mapName);
	void SetRating(std::shared_ptr<UI::RenderData<unsigned int>> rating);
	void SetPadding(float padding) { _padding = padding; }
	void SetLocked(bool locked) { _locked = locked; }

	// UI::Window
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const override;
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;

private:
	FS::FileSystem &_fs;
	WorldView &_worldView;
	MapCollection &_mapCollection;
	UI::Texture _texSelection = "ui/selection";
	UI::Texture _texLock = "ui/lock";
	UI::Texture _texLockShade = "ui/window";
	std::shared_ptr<UI::Rating> _rating;
	std::shared_ptr<UI::RenderData<std::string_view>> _mapName;
	float _padding = 0;
	bool _locked = false;
};
