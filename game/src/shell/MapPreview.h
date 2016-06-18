#pragma once
#include <ui/Button.h>
#include <memory>
#include <string>
class World;
class WorldView;
namespace FS
{
	class FileSystem;
}

class MapPreview
	: public UI::ButtonBase
{
public:
	MapPreview(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView);

	void SetMapName(std::string mapName, FS::FileSystem &fs);

	// UI::Window
	void Draw(bool hovered, bool focused, bool enabled, vec2d size, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	std::unique_ptr<World> _world;
	WorldView &_worldView;
	std::string _title;
	size_t _font;
};
