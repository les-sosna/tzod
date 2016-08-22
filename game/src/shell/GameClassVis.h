#pragma once
#include <ui/Window.h>
#include <gc/World.h>
#include <functional>

class WorldView;

class GameClassVis : public UI::Window
{
	std::function<const std::string&(const UI::StateContext&)> _dataBinding;

public:
	GameClassVis(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView);

	void SetGameClass(unsigned int type);
	void SetDataBinding(decltype(_dataBinding) dataBinding) { _dataBinding = std::move(dataBinding); }

	// UI::Window
	void Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	WorldView &_worldView;
	mutable World _world;
	size_t _texSelection;
};
