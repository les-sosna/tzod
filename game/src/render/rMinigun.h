#pragma once
#include "inc/render/ObjectView.h"
#include <stddef.h> // size_t

class TextureManager;

class R_WeaponMinigun : public ObjectRFunc
{
public:
	R_WeaponMinigun(TextureManager &tm);

	// ObjectView
	void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

private:
	size_t _texId1;
	size_t _texId2;
};

class R_Crosshair2 : public ObjectRFunc
{
public:
	R_Crosshair2(TextureManager &tm);
	void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

private:
	size_t _texId;
};
