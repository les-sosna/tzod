#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_Turret : public ObjectView
{
public:
	R_Turret(TextureManager &tm, const char *texPlatform, const char *texWeapon);
	
	// ObjectView
	virtual enumZOrder GetZ(const GC_Actor &actor) const { return Z_WALLS; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texPlatform;
	size_t _texWeapon;
};
