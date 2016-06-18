#pragma once
#include <ui/Window.h>
#include <memory>
#include <string>
class World;
class WorldView;
namespace FS
{
	class FileSystem;
}

class MapPreview
	: public UI::Window
	, private UI::PointerSink
{
public:
	MapPreview(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView);

	void SetMapName(std::string mapName, FS::FileSystem &fs);

	// UI::Window
	void Draw(bool hovered, bool focused, bool enabled, vec2d size, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	UI::PointerSink* GetPointerSink() override { return this; }

private:
	std::unique_ptr<World> _world;
	WorldView &_worldView;
	std::string _title;
	size_t _font;
};
