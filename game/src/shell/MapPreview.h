#pragma once
#include <ui/Texture.h>
#include <ui/Window.h>
#include <memory>
#include <string>
class WorldView;
class MapCache;

namespace FS
{
	class FileSystem;
}
namespace UI
{
	class Rating;
	template<class T> struct RenderData;
}

class MapPreview: public UI::Window
{
public:
	MapPreview(FS::FileSystem &fs, WorldView &worldView, MapCache &mapCache);

	void SetMapName(std::shared_ptr<UI::RenderData<const std::string&>> mapName);
	void SetRating(std::shared_ptr<UI::RenderData<unsigned int>> rating);
	void SetPadding(float padding) { _padding = padding; }

	// UI::Window
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;

private:
	FS::FileSystem &_fs;
	WorldView &_worldView;
	MapCache &_mapCache;
	UI::Texture _texSelection = "ui/selection";
	UI::Texture _texLock = "ui/lock";
	UI::Texture _texLockShade = "ui/window";
	std::shared_ptr<UI::Rating> _rating;
	std::shared_ptr<UI::RenderData<const std::string&>> _mapName;
	float _padding = 0;
};
