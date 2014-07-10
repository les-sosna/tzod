#pragma once
#include "rWeaponBase.h"

#include <stddef.h>

class TextureManager;

class R_Weapon : public R_WeaponBase
{
public:
	R_Weapon(TextureManager &tm, const char *tex);
	
	// ObjectView
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};

class R_WeapFireEffect : public ObjectView
{
public:
	R_WeapFireEffect(TextureManager &tm, const char *tex, float duration, float offsetX, bool oriented);
	
	// ObjectView
	virtual enumZOrder GetZ(World &world, const GC_Actor &actor) const override;
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texId;
	float _duration;
	float _offsetX;
	bool _oriented;
};

class R_RipperDisk : public ObjectView
{
public:
	R_RipperDisk(TextureManager &tm);
	
	// ObjectView
	virtual enumZOrder GetZ(World &world, const GC_Actor &actor) const override { return Z_PROJECTILE; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};

class R_Crosshair : public ObjectView
{
public:
	R_Crosshair(TextureManager &tm);
	
	// ObjectView
	virtual enumZOrder GetZ(World &world, const GC_Actor &actor) const { return Z_VEHICLE_LABEL; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};
