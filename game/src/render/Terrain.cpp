#include "inc/render/Terrain.h"
#include <video/RenderBase.h>
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

Terrain::Terrain(TextureManager &tm)
	: _texBack(tm.FindSprite("background"))
	, _texGrid(tm.FindSprite("grid"))
{
}

void Terrain::Draw(DrawingContext &dc, const FRECT &bounds, bool drawGrid) const
{
	dc.DrawBackground(_texBack, bounds);
	if( drawGrid )
		dc.DrawBackground(_texGrid, bounds);
}
