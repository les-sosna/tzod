#include "Terrain.h"
#include "config/Config.h"
#include "video/RenderBase.h"
#include "video/TextureManager.h"

Terrain::Terrain(TextureManager &tm)
	: _tm(tm)
	, _texBack(tm.FindSprite("background"))
	, _texGrid(tm.FindSprite("grid"))
{
}

void Terrain::Draw(IRender &render, float sizeX, float sizeY, bool editorMode) const
{
	DrawBackground(render, sizeX, sizeY, _texBack);
	if( editorMode && g_conf.ed_drawgrid.Get() )
		DrawBackground(render, sizeX, sizeY, _texGrid);
}

void Terrain::DrawBackground(IRender &render, float sizeX, float sizeY, size_t tex) const
{
	const LogicalTexture &lt = _tm.Get(tex);
	MyVertex *v = render.DrawQuad(lt.dev_texture);
	v[0].color = 0xffffffff;
	v[0].u = 0;
	v[0].v = 0;
	v[0].x = 0;
	v[0].y = 0;
	v[1].color = 0xffffffff;
	v[1].u = sizeX / lt.pxFrameWidth;
	v[1].v = 0;
	v[1].x = sizeX;
	v[1].y = 0;
	v[2].color = 0xffffffff;
	v[2].u = sizeX / lt.pxFrameWidth;
	v[2].v = sizeY / lt.pxFrameHeight;
	v[2].x = sizeX;
	v[2].y = sizeY;
	v[3].color = 0xffffffff;
	v[3].u = 0;
	v[3].v = sizeY / lt.pxFrameHeight;
	v[3].x = 0;
	v[3].y = sizeY;
}
