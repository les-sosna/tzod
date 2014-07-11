#pragma once
#include "ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_Weapon : public ObjectRFunc
{
public:
	R_Weapon(TextureManager &tm, const char *tex);
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};

class R_WeapFireEffect : public ObjectRFunc
{
public:
	R_WeapFireEffect(TextureManager &tm, const char *tex, float duration, float offsetX, bool oriented);
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texId;
	float _duration;
	float _offsetX;
	bool _oriented;
};

class Z_WeapFireEffect : public ObjectZFunc
{
public:
	Z_WeapFireEffect(float duration) : _duration(duration) {}
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const override;

private:
	float _duration;
};

class R_RipperDisk : public ObjectRFunc
{
public:
	R_RipperDisk(TextureManager &tm);
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};

class R_Crosshair : public ObjectRFunc
{
public:
	R_Crosshair(TextureManager &tm);
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};
