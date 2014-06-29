#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_HealthIndicator : public ObjectView
{
public:
	R_HealthIndicator(TextureManager &tm);
	
	// ObjectView
	virtual enumZOrder GetZ(World &world, const GC_Actor &actor) const { return Z_VEHICLE_LABEL; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texId;
};

class R_AmmoIndicator : public ObjectView
{
public:
	R_AmmoIndicator(TextureManager &tm);
	
	// ObjectView
	virtual enumZOrder GetZ(World &world, const GC_Actor &actor) const { return Z_VEHICLE_LABEL; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texId;
};

class R_FuelIndicator : public ObjectView
{
public:
	R_FuelIndicator(TextureManager &tm);
	
	// ObjectView
	virtual enumZOrder GetZ(World &world, const GC_Actor &actor) const { return Z_VEHICLE_LABEL; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texId;
};
