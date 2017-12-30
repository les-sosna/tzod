#pragma once
#include <ui/Texture.h>
#include <ui/Window.h>
#include <memory>
#include <string>
class WorldView;
class WorldCache;

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
	MapPreview(FS::FileSystem &fs, WorldView &worldView, WorldCache &mapCache);

	void SetMapName(std::shared_ptr<UI::RenderData<std::string_view>> mapName);
	void SetRating(std::shared_ptr<UI::RenderData<unsigned int>> rating);
	void SetPadding(float padding) { _padding = padding; }
	void SetLocked(bool locked) { _locked = locked; }

	// UI::Window
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;

private:
	FS::FileSystem &_fs;
	WorldView &_worldView;
	WorldCache &_worldCache;
	UI::Texture _texSelection = "ui/selection";
	UI::Texture _texLock = "ui/lock";
	UI::Texture _texLockShade = "ui/window";
	std::shared_ptr<UI::Rating> _rating;
	std::shared_ptr<UI::RenderData<std::string_view>> _mapName;
	float _padding = 0;
	bool _locked = false;
};
