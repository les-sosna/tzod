#pragma once
class DrawingContext;
class GC_Actor;
class World;

enum enumZOrder
{
	Z_EDITOR,           // editor labels
	Z_WATER,            // water
	Z_GAUSS_RAY,        // gauss ray
	Z_WALLS,            // walls
	Z_FREE_ITEM,        // not picked up item
	Z_VEHICLES,         // vehicles
	Z_ATTACHED_ITEM,    // picked up items (weapon etc.)
	Z_PROJECTILE,       // flying projectiles
	Z_EXPLODE,          // explosions
	Z_VEHICLE_LABEL,    // vehicle labels (the crosshair, the health bar, etc.)
	Z_PARTICLE,         // particles, smoke
	Z_WOOD,             // wood
	//------------------//
	Z_COUNT,            // total number of z-layers
	//------------------//
	Z_NONE = -1         // not drawn
};

struct ObjectZFunc
{
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const = 0;
	virtual ~ObjectZFunc() {}
};

struct ObjectRFunc
{
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const = 0;
	virtual ~ObjectRFunc() {}
};
