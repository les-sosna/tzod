#include "rUserObject.h"
#include <gc/UserObjects.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

R_UserObject::R_UserObject(TextureManager &tm)
	: _tm(tm)
{
}

void R_UserObject::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_UserObject*>(&mo));
	auto &userObject = static_cast<const GC_UserObject&>(mo);

	vec2d pos = userObject.GetPos();
	vec2d dir = userObject.GetDirection();
	size_t texId = _tm.FindSprite(userObject.GetTextureName());
	rc.DrawSprite(texId, 0, 0xffffffff, pos, dir);
}

enumZOrder Z_UserObject::GetZ(const World &world, const GC_MovingObject &mo) const
{
	assert(dynamic_cast<const GC_UserObject*>(&mo));
	auto &userObject = static_cast<const GC_UserObject&>(mo);
	return userObject.GetZ();
}

