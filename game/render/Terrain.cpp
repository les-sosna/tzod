#include "inc/render/Terrain.h"
#include <gc/World.h>
#include <gc/WorldCfg.h>
#include <video/RenderBase.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

Terrain::Terrain(TextureManager &tm)
	: _texBack(tm.FindSprite("background"))
	, _texGrid(tm.FindSprite("grid"))
	, _texWater(tm.FindSprite("water"))
{
}

static const int dx[8] = { 1,  0, -1,    1, -1,    1,  0, -1 };
static const int dy[8] = { 1,  1,  1,    0,  0,   -1, -1, -1 };

void Terrain::Draw(RenderContext &rc, const World& world, bool drawGrid) const
{
	auto bounds = world.GetBounds();
	rc.DrawBackground(_texBack, bounds);

	if( drawGrid && rc.GetScale() > 0.25 )
		rc.DrawBackground(_texGrid, bounds);

	if (rc.GetScale() <= 0.25)
		return;

	FRECT visibleRegion = rc.GetVisibleRegion();
	int xmin = std::max(world.GetBlockBounds().left, (int)std::floor(visibleRegion.left / WORLD_BLOCK_SIZE - 0.5f));
	int ymin = std::max(world.GetBlockBounds().top, (int)std::floor(visibleRegion.top / WORLD_BLOCK_SIZE - 0.5f));
	int xmax = std::min(world.GetBlockBounds().right - 1, (int)std::floor(visibleRegion.right / WORLD_BLOCK_SIZE + 0.5f));
	int ymax = std::min(world.GetBlockBounds().bottom - 1, (int)std::floor(visibleRegion.bottom / WORLD_BLOCK_SIZE + 0.5f));

	struct TileDef
	{
		int frame;
		int include;
		int exclude;
	};

	constexpr int LT = 0b00000001, CT = 0b00000010, RT = 0b00000100;
	constexpr int LC = 0b00001000,                  RC = 0b00010000;
	constexpr int LB = 0b00100000, CB = 0b01000000, RB = 0b10000000;

	TileDef definitions[] =
	{
		{ 0, LT, CT|LC },
		{ 1, CT, /*LC|RC*/ },
		{ 2, RT, CT|RC },
		{ 3, LC, /*CT|CB*/ },
		{ 5, RC, /*CT|CB*/ },
		{ 6, LB, LC|CB },
		{ 7, CB, /*LC|RC*/ },
		{ 8, RB, CB|RC },
	};


	for( int y0 = ymin; y0 <= ymax; ++y0 )
	for( int x0 = xmin; x0 <= xmax; ++x0 )
	{
		if (int tileIndex = world.GetTileIndex(x0, y0); tileIndex != -1 && world._waterTiles[tileIndex])
			continue;

		int neighbors = 0;
		for (int i = 0; i < 8; i++)
		{
			auto x = x0 + dx[i];
			auto y = y0 + dy[i];
			auto tileIndex = world.GetTileIndex(x, y);
			if (tileIndex != -1 && world._waterTiles[tileIndex])
			{
				neighbors |= 1u << i;
			}
		}

		if (neighbors)
		{
			auto rect = MakeRectWH(vec2d{ (float)x0, (float)y0 } *WORLD_BLOCK_SIZE, vec2d{ WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE });
			for (auto def : definitions)
			{
				if ((neighbors & def.include) && !(neighbors & def.exclude))
				{
					rc.DrawSprite(rect, _texWater, 0xffffffff, def.frame);
				}
			}
		}
	}
}
