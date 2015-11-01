#pragma once
#include "inc/render/ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_Turret : public ObjectRFunc
{
public:
	R_Turret(TextureManager &tm, const char *texPlatform, const char *texWeapon);
	void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

private:
	TextureManager &_tm;
	size_t _texPlatform;
	size_t _texWeapon;
};
