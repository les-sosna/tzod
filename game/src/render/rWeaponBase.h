#pragma once
#include "inc/render/ObjectView.h"
#include <math/MyMath.h>
#include <stddef.h>

class DrawingContext;
class GC_Weapon;

vec2d GetWeapSpriteDirection(const World &world, const GC_Weapon &weapon);
void DrawWeaponShadow(const World &world, const GC_Weapon &weapon, DrawingContext &dc, size_t texId);

class Z_Weapon : public ObjectZFunc
{
public:
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const override;
};
