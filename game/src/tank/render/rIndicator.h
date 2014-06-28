#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_IndicatorBase : public ObjectView
{
public:
	// ObjectView
	virtual enumZOrder GetZ(const GC_Actor &actor) const { return Z_VEHICLE_LABEL; }
};

class R_HealthIndicator : public R_IndicatorBase
{
public:
	R_HealthIndicator(TextureManager &tm);
	
	// ObjectView
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texId;
};

class R_AmmoIndicator : public R_IndicatorBase
{
public:
	R_AmmoIndicator(TextureManager &tm);
	
	// ObjectView
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texId;
};

class R_FuelIndicator : public R_IndicatorBase
{
public:
	R_FuelIndicator(TextureManager &tm);
	
	// ObjectView
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texId;
};
