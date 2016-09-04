#pragma once
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
	struct TextSource;
}

class MapPreview: public UI::Window
{
public:
	MapPreview(UI::LayoutManager &manager, TextureManager &texman, FS::FileSystem &fs, WorldView &worldView, MapCache &mapCache);

	void SetMapName(std::shared_ptr<UI::TextSource> mapName);
	void SetPadding(float padding) { _padding = padding; }

	// UI::Window
	void Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const override;

private:
	FS::FileSystem &_fs;
	WorldView &_worldView;
	MapCache &_mapCache;
	size_t _font;
	size_t _texSelection;
	std::shared_ptr<UI::Rating> _rating;
	std::shared_ptr<UI::TextSource> _mapName;
	float _padding = 0;
};
