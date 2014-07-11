#pragma once
#include "ObjectView.h"

#include <stddef.h>

class DrawingContext;
class GC_Weapon;

void DrawWeaponShadow(DrawingContext &dc, size_t texId, const GC_Weapon &weapon);

class Z_Weapon : public ObjectZFunc
{
public:
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const override;
};
