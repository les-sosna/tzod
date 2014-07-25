#include "rParticle.h"
#include "gc/particles.h"

static std::pair<ParticleType, const char*> textures[] = {
	{ PARTICLE_TYPE1, "particle_1" },
};

R_Particle::R_Particle(TextureManager &tm)
	: _tm(tm)
{
	int maxId = 0;
	for (auto p: textures)
		maxId = std::max(maxId, (int) p.first);
	_ptype2texId.resize(maxId + 1);
	for (auto p: textures)
		_ptype2texId[p.first] = tm.FindSprite(p.second);
}

void R_Particle::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Particle*>(&actor));
	const GC_Particle &particle = static_cast<const GC_Particle&>(actor);
	ParticleType ptype = particle.GetParticleType();
	if (ptype < _ptype2texId.size() && particle.GetTime() < particle.GetLifeTime())
	{
		size_t texId = _ptype2texId[ptype];
		float state = particle.GetTime() / particle.GetLifeTime();
		auto frame = std::min(_tm.GetFrameCount(texId) - 1, (unsigned int) ((float) _tm.GetFrameCount(texId) * state));
		vec2d pos = particle.GetPos();
		vec2d dir = particle.GetDirection();
		SpriteColor color;
		if (particle.GetFade())
		{
			unsigned char op = (unsigned char) int(255.0f * (1.0f - state));
			color.r = op;
			color.g = op;
			color.b = op;
			color.a = op;
		}
		else
		{
			color = 0xffffffff;
		}
		dc.DrawSprite(texId, frame, color, pos.x, pos.y, dir);
	}
}
