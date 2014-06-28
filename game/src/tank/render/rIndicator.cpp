#include "rIndicator.h"
#include "gc/RigidBody.h"
#include "video/TextureManager.h"


R_HealthIndicator::R_HealthIndicator(TextureManager &tm)
	: _tm(tm)
	, _texId(tm.FindSprite("indicator_health"))
{
}

void R_HealthIndicator::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_RigidBodyStatic*>(&actor));
	auto &rigidBody = static_cast<const GC_RigidBodyStatic&>(actor);
	
	vec2d pos = rigidBody.GetPos();
	float val = rigidBody.GetHealth() / rigidBody.GetHealthMax();
	FRECT rt;
	rigidBody.GetLocalRect(rt);
	float top = pos.y + rt.top;
	dc.DrawIndicator(_texId, pos.x, std::max(top - _tm.GetFrameHeight(_texId, 0), .0f), val);
}
