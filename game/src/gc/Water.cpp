#include "TypeReg.h"
#include "inc/gc/SaveFile.h"
#include "inc/gc/Water.h"
#include "inc/gc/WorldCfg.h"

IMPLEMENT_SELF_REGISTRATION(GC_Water)
{
	ED_LAND( "water", "obj_water", 0 );
	return true;
}

GC_Water::GC_Water(vec2d pos)
  : GC_RigidBodyStatic(pos)
{
	SetSize(CELL_SIZE, CELL_SIZE);
	SetFlags(GC_FLAG_RBSTATIC_TRACE0, true);
}

GC_Water::GC_Water(FromFile)
  : GC_RigidBodyStatic(FromFile())
{
}

GC_Water::~GC_Water()
{
}

void GC_Water::MoveTo(World &world, const vec2d &pos)
{
	int oldTile = world.GetTileIndex(GetPos());
	int newTile = world.GetTileIndex(pos);
	if (oldTile != newTile)
	{
		if (-1 != oldTile)
			world._waterTiles[oldTile] = false;
		if (-1 != newTile)
			world._waterTiles[newTile] = true;
	}
	GC_RigidBodyStatic::MoveTo(world, pos);
}

void GC_Water::Init(World &world)
{
	GC_RigidBodyStatic::Init(world);
	int tileIndex = world.GetTileIndex(GetPos());
	if (-1 != tileIndex)
	{
		world._waterTiles[tileIndex] = true;
	}
}

void GC_Water::Kill(World &world)
{
	int tileIndex = world.GetTileIndex(GetPos());
	if (-1 != tileIndex)
	{
		world._waterTiles[tileIndex] = false;
	}
	GC_RigidBodyStatic::Kill(world);
}

static const int dx[8] = { 1,   1,   0,  -1,  -1,  -1,   0,   1 };
static const int dy[8] = { 0,   1,   1,   1,   0,  -1,  -1,  -1 };
static const int nf[8] = { 131,   2,  14,   8,  56,  32, 224, 128 };

int GC_Water::GetNeighbors(const World &world) const
{
	int x0 = int(GetPos().x / CELL_SIZE);
	int y0 = int(GetPos().y / CELL_SIZE);

	int neighbors = 0;
	for (int i = 0; i < 8; i++)
	{
		auto x = x0 + dx[i];
		auto y = y0 + dy[i];
		auto tileIndex = (x >= 0 && x < world._cellsX && y >= 0 && y < world._cellsY) ? x + world._cellsX * y : -1;
		if (tileIndex == -1 || world._waterTiles[tileIndex])
		{
			neighbors |= nf[i];
		}
	}

	return neighbors;
}

void GC_Water::OnDamage(World &world, DamageDesc &dd)
{
	dd.damage = 0;
}
