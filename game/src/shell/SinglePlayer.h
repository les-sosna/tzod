#pragma once
#include <ui/Dialog.h>

class World;
class WorldView;
namespace FS
{
	class FileSystem;
}

class SinglePlayer : public UI::Dialog
{
public:
	SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs);

	void Draw(bool hovered, bool focused, bool enabled, vec2d size, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	std::unique_ptr<World> _world;
	WorldView &_worldView;
};
