#include "inc/render/Terrain.h"
#include <video/RenderBase.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

Terrain::Terrain(TextureManager &tm)
	: _texBack(tm.FindSprite("background"))
	, _texGrid(tm.FindSprite("grid"))
{
}

void Terrain::Draw(RenderContext &rc, const FRECT &bounds, bool drawGrid) const
{
	rc.DrawBackground(_texBack, bounds);
	if( drawGrid && rc.GetScale() > 0.25 )
		rc.DrawBackground(_texGrid, bounds);
}
