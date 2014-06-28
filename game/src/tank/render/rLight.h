#pragma once

#include "ObjectView.h"

#include <stddef.h>

class TextureManager;
struct IRender;
class GC_Light;

void DrawLight(IRender &render, const GC_Light &light);

class R_Light : public ObjectView
{
public:
	R_Light(TextureManager &tm);
	
	// ObjectView
	virtual enumZOrder GetZ(const GC_Actor &actor) const;
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};
