#pragma once
#include <ui/Window.h>
#include <gc/World.h>

class WorldView;

class GameClassVis : public UI::Window
{
public:
	GameClassVis(UI::LayoutManager &manager, WorldView &worldView);

	void SetGameClass(unsigned int type);

	void Draw(const UI::LayoutContext &lc, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	WorldView &_worldView;
	World _world;
};
