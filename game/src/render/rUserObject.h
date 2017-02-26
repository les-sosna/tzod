#pragma once
#include "inc/render/ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_UserObject : public ObjectRFunc
{
public:
	R_UserObject(TextureManager &tm);
	void Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const override;

private:
	TextureManager &_tm;
};

class Z_UserObject : public ObjectZFunc
{
public:
	enumZOrder GetZ(const World &world, const GC_Actor &actor) const override;
};
