#include "Terrain.h"
#include "config/Config.h"
#include "video/RenderBase.h"
#include "video/TextureManager.h"

Terrain::Terrain(TextureManager &tm)
	: _texBack(tm.FindSprite("background"))
	, _texGrid(tm.FindSprite("grid"))
{
}

void Terrain::Draw(DrawingContext &dc, float sizeX, float sizeY, bool editorMode) const
{
	dc.DrawBackground(_texBack, sizeX, sizeY);
	if( editorMode && g_conf.ed_drawgrid.Get() )
		dc.DrawBackground(_texGrid, sizeX, sizeY);
}
