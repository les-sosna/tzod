#pragma once
#include "inc/render/ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_HealthIndicator : public ObjectRFunc
{
public:
	R_HealthIndicator(TextureManager &tm, bool dynamic);
	void Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const override;

private:
	TextureManager &_tm;
	size_t _texId;
	bool _dynamic;
};

class R_AmmoIndicator : public ObjectRFunc
{
public:
	R_AmmoIndicator(TextureManager &tm);
	void Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const override;

private:
	TextureManager &_tm;
	size_t _texId;
};

class R_FuelIndicator : public ObjectRFunc
{
public:
	R_FuelIndicator(TextureManager &tm);
	void Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const override;

private:
	TextureManager &_tm;
	size_t _texId;
};
