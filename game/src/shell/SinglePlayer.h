#pragma once
#include <ui/Dialog.h>
#include <thread>

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
};
