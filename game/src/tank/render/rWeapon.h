#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_Weapon : public ObjectView
{
public:
	R_Weapon(TextureManager &tm, const char *tex);
	
	// ObjectView
	virtual enumZOrder GetZ(const GC_Actor &actor) const;
	virtual void Draw(const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};
