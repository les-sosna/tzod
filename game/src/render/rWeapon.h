#pragma once
#include "inc/render/ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_Weapon : public ObjectRFunc
{
public:
	R_Weapon(TextureManager &tm, const char *tex);
	void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

private:
	size_t _texId;
};

class R_WeapFireEffect : public ObjectRFunc
{
public:
	R_WeapFireEffect(TextureManager &tm, const char *tex, float duration, float offsetX, bool oriented);
	void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

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
	enumZOrder GetZ(const World &world, const GC_Actor &actor) const override;

private:
	float _duration;
};

class R_RipperDisk : public ObjectRFunc
{
public:
	R_RipperDisk(TextureManager &tm);
	void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

private:
	size_t _texId;
};

class R_Crosshair : public ObjectRFunc
{
public:
	R_Crosshair(TextureManager &tm);
	void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

private:
	size_t _texId;
};
