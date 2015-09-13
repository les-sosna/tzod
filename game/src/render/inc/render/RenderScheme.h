#pragma once
#include "ObjectViewsSelector.h"

class TextureManager;
class GC_Actor;

class RenderScheme
{
public:
	RenderScheme(TextureManager &tm);
	const ObjectViewsSelector::ViewCollection* GetViews(const GC_Actor &actor, bool editorMode, bool nightMode) const;

private:
	ObjectViewsSelector _gameViews;
	ObjectViewsSelector _editorViews;
	ObjectViewsSelector _nightViews;
};
