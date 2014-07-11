#pragma once

#include "ObjectView.h"

#include <stddef.h>

class TextureManager;
struct IRender;
class GC_Light;

void DrawLight(IRender &render, const GC_Light &light);

class R_Light : public ObjectRFunc
{
public:
	R_Light(TextureManager &tm);
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};

class Z_Light : public ObjectZFunc
{
public:
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const;
};
