#include "rLight.h"
#include "gc/Light.h"
#include "config/Config.h"
#include "video/RenderBase.h"
#include "video/TextureManager.h"

R_Light::R_Light(TextureManager &tm)
	: _texId(tm.FindSprite("shine"))
{
}

enumZOrder R_Light::GetZ(const GC_Actor &actor) const
{
	assert(dynamic_cast<const GC_Light*>(&actor));
	auto &light = static_cast<const GC_Light&>(actor);
	return GC_Light::LIGHT_SPOT == light.GetLightType()
		&& light.GetActive() && g_conf.sv_nightmode.Get() ? Z_PARTICLE : Z_NONE;
}
	
void R_Light::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Light*>(&actor));
	auto &light = static_cast<const GC_Light&>(actor);
	vec2d pos = light.GetPos();
	dc.DrawSprite(_texId, 0, 0xffffffff, pos.x, pos.y, vec2d(0, 1));
}



static const int SINTABLE_SIZE = 32;
static const int SINTABLE_MASK = 0x1f;

static float sintable[SINTABLE_SIZE] = {
	 0.000000f, 0.195090f, 0.382683f, 0.555570f,
	 0.707106f, 0.831469f, 0.923879f, 0.980785f,
	 1.000000f, 0.980785f, 0.923879f, 0.831469f,
	 0.707106f, 0.555570f, 0.382683f, 0.195090f,
	-0.000000f,-0.195090f,-0.382683f,-0.555570f,
	-0.707106f,-0.831469f,-0.923879f,-0.980785f,
	-1.000000f,-0.980785f,-0.923879f,-0.831469f,
	-0.707106f,-0.555570f,-0.382683f,-0.195090f,
};

void DrawLight(IRender &render, const GC_Light &light)
{
	MyVertex *v;

	SpriteColor color;
	color.color = 0x00000000;
	color.a = (unsigned char) std::max(0, std::min(255, int(255.0f * light.GetIntensity())));
	
	vec2d pos = light.GetPos();
	vec2d dir = light.GetLightDirection();

	switch( light.GetLightType() )
	{
	case GC_Light::LIGHT_POINT:
		v = render.DrawFan(SINTABLE_SIZE>>1);
		v[0].color = color;
		v[0].x = pos.x;
		v[0].y = pos.y;
		for( int i = 0; i < SINTABLE_SIZE>>1; i++ )
		{
			v[i+1].x = pos.x + light.GetRadius() * sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			v[i+1].y = pos.y + light.GetRadius() * sintable[i<<1];
			v[i+1].color.color = 0x00000000;
		}
		break;
	case GC_Light::LIGHT_SPOT:
		v = render.DrawFan(SINTABLE_SIZE);
		v[0].color = color;
		v[0].x = pos.x;
		v[0].y = pos.y;
		for( int i = 0; i < SINTABLE_SIZE; i++ )
		{
			float x = light.GetOffset() + light.GetRadius() * sintable[i+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			float y =                     light.GetRadius() * sintable[i] * light.GetAspect();
			v[i+1].x = pos.x + x*dir.x - y*dir.y;
			v[i+1].y = pos.y + y*dir.x + x*dir.y;
			v[i+1].color.color = 0x00000000;
		}
		break;
	case GC_Light::LIGHT_DIRECT:
		v = render.DrawFan((SINTABLE_SIZE>>2)+4);
		v[0].color = color;
		v[0].x = pos.x;
		v[0].y = pos.y;
		for( int i = 0; i <= SINTABLE_SIZE>>2; i++ )
		{
			float x = light.GetRadius() * sintable[i<<1];
			float y = light.GetRadius() * sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			v[i+1].x = pos.x - x*dir.x - y*dir.y;
			v[i+1].y = pos.y - x*dir.y + y*dir.x;
			v[i+1].color.color = 0x00000000;
		}

		v[(SINTABLE_SIZE>>2)+2].color.color = 0x00000000;
		v[(SINTABLE_SIZE>>2)+2].x = pos.x + light.GetLength() * dir.x + light.GetRadius()*dir.y;
		v[(SINTABLE_SIZE>>2)+2].y = pos.y + light.GetLength() * dir.y - light.GetRadius()*dir.x;

		v[(SINTABLE_SIZE>>2)+3].color = color;
		v[(SINTABLE_SIZE>>2)+3].x = pos.x + light.GetLength() * dir.x;
		v[(SINTABLE_SIZE>>2)+3].y = pos.y + light.GetLength() * dir.y;

		v[(SINTABLE_SIZE>>2)+4].color.color = 0x00000000;
		v[(SINTABLE_SIZE>>2)+4].x = pos.x + light.GetLength() * dir.x - light.GetRadius()*dir.y;
		v[(SINTABLE_SIZE>>2)+4].y = pos.y + light.GetLength() * dir.y + light.GetRadius()*dir.x;

		v = render.DrawFan((SINTABLE_SIZE>>2)+1);
		v[0].color = color;
		v[0].x = pos.x + light.GetLength() * dir.x;
		v[0].y = pos.y + light.GetLength() * dir.y;
		for( int i = 0; i <= SINTABLE_SIZE>>2; i++ )
		{
			float x = light.GetRadius() * sintable[i<<1] + light.GetLength();
			float y = light.GetRadius() * sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			v[i+1].x = pos.x + x*dir.x - y*dir.y;
			v[i+1].y = pos.y + x*dir.y + y*dir.x;
			v[i+1].color.color = 0x00000000;
		}
		break;
	default:
		assert(false);
	}
}
