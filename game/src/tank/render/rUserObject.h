#pragma once
#include "ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_UserObject : public ObjectRFunc
{
public:
	R_UserObject(TextureManager &tm);
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
};

class Z_UserObject : public ObjectZFunc
{
public:
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const override;
};
