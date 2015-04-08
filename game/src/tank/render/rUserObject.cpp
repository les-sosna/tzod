#include "rUserObject.h"
#include <gc/UserObjects.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>

R_UserObject::R_UserObject(TextureManager &tm)
	: _tm(tm)
{
}
	
void R_UserObject::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_UserObject*>(&actor));
	auto &userObject = static_cast<const GC_UserObject&>(actor);
	
	vec2d pos = userObject.GetPos();
	vec2d dir = userObject.GetDirection();
	size_t texId = _tm.FindSprite(userObject.GetTextureName());
	dc.DrawSprite(texId, 0, 0xffffffff, pos.x, pos.y, dir);
}

enumZOrder Z_UserObject::GetZ(const World &world, const GC_Actor &actor) const
{
	assert(dynamic_cast<const GC_UserObject*>(&actor));
	auto &userObject = static_cast<const GC_UserObject&>(actor);
	return userObject.GetZ();
}

