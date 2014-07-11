#pragma once

#include "ObjectViewsSelector.h"
#include "Terrain.h"

class TextureManager;
class World;
class GC_Actor;
struct FRECT;
struct IRender;

class WorldView
{
public:
    WorldView(IRender &render, TextureManager &texman);
	~WorldView();
	void Render(World &world, const FRECT &view, bool editorMode) const;

private:
    IRender &_render;
    TextureManager &_tm;
	Terrain _terrain;
	ObjectViewsSelector _gameViews;
	ObjectViewsSelector _editorViews;
	inline const ObjectViewsSelector::ViewCollection* GetViews(const GC_Actor &actor, bool editorMode) const;
};
