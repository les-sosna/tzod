#include "TypeReg.h"
#include "inc/gc/SaveFile.h"
#include "inc/gc/Water.h"
#include "inc/gc/WorldCfg.h"

IMPLEMENT_SELF_REGISTRATION(GC_Water)
{
	ED_LAND( "water", "obj_water", 0 );
	return true;
}

IMPLEMENT_GRID_MEMBER(GC_Water, grid_water);

GC_Water::GC_Water(vec2d pos)
  : GC_RigidBodyStatic(pos)
  , _tile(0)
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
    if( CheckFlags(GC_FLAG_WATER_INTILE) )
        UpdateTile(world, false);
    GC_RigidBodyStatic::MoveTo(world, pos);
    UpdateTile(world, true);
    SetFlags(GC_FLAG_WATER_INTILE, true);
}

void GC_Water::Kill(World &world)
{
    if( CheckFlags(GC_FLAG_WATER_INTILE) )
        UpdateTile(world, false);
    GC_RigidBodyStatic::Kill(world);
}

void GC_Water::UpdateTile(World &world, bool flag)
{
	static char tile1[9] = {5, 6, 7, 4,-1, 0, 3, 2, 1};
	static char tile2[9] = {1, 2, 3, 0,-1, 4, 7, 6, 5};

	FRECT frect;
	frect.left   = (GetPos().x - CELL_SIZE / 2) / LOCATION_SIZE - 0.5f;
	frect.top    = (GetPos().y - CELL_SIZE / 2) / LOCATION_SIZE - 0.5f;
	frect.right  = (GetPos().x + CELL_SIZE / 2) / LOCATION_SIZE - 0.5f;
	frect.bottom = (GetPos().y + CELL_SIZE / 2) / LOCATION_SIZE - 0.5f;

    std::vector<ObjectList*> receive;
	world.grid_water.OverlapRect(receive, frect);

	for( auto rit = receive.begin(); rit != receive.end(); ++rit )
	{
        ObjectList *ls = *rit;
		for( auto it = ls->begin(); it != ls->end(); it = ls->next(it) )
		{
			GC_Water *object = (GC_Water *) ls->at(it);
			if( this == object ) continue;

			vec2d dx = (GetPos() - object->GetPos()) / CELL_SIZE;
			if( dx.sqr() < 2.5f )
			{
				int x = int(dx.x + 1.5f);
				int y = int(dx.y + 1.5f);

				object->SetTile(tile1[x + y * 3], flag);
				SetTile(tile2[x + y * 3], flag);
			}
		}
	}
}

void GC_Water::Serialize(World &world, SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(world, f);
	f.Serialize(_tile);
}

void GC_Water::SetTile(char nTile, bool value)
{
	assert(0 <= nTile && nTile < 8);

	if( value )
		_tile |= (1 << nTile);
	else
		_tile &= ~(1 << nTile);
}

void GC_Water::OnDamage(World &world, DamageDesc &dd)
{
	dd.damage = 0;
}
