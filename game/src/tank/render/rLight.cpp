#include "rLight.h"
#include "gc/Light.h"
#include "config/Config.h"
#include "video/RenderBase.h"
#include "video/TextureManager.h"

enumZOrder Z_Light::GetZ(const World &world, const GC_Actor &actor) const
{
	assert(dynamic_cast<const GC_Light*>(&actor));
	auto &light = static_cast<const GC_Light&>(actor);
	return GC_Light::LIGHT_SPOT == light.GetLightType()
		&& light.GetActive() && g_conf.sv_nightmode.Get() ? Z_PARTICLE : Z_NONE;
}
	
R_Light::R_Light(TextureManager &tm)
	: _texId(tm.FindSprite("shine"))
{
}

void R_Light::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Light*>(&actor));
	auto &light = static_cast<const GC_Light&>(actor);
	vec2d pos = light.GetPos();
	dc.DrawSprite(_texId, 0, 0xffffffff, pos.x, pos.y, vec2d(0, 1));
}
