#include "rParticle.h"
#include "gc/particles.h"
#include "video/TextureManager.h"
#include "video/DrawingContext.h"

static std::pair<ParticleType, const char*> textures[] = {
	{ PARTICLE_FIRE1, "particle_fire" },
	{ PARTICLE_FIRE2, "particle_fire2" },
	{ PARTICLE_FIRE3, "particle_fire3" },
	{ PARTICLE_FIRE4, "particle_fire4" },
	{ PARTICLE_FIRESPARK, "projectile_fire" },
	{ PARTICLE_TYPE1, "particle_1" },
	{ PARTICLE_TYPE2, "particle_2" },
	{ PARTICLE_TYPE3, "particle_3" },
	{ PARTICLE_TRACE1, "particle_trace" },
	{ PARTICLE_TRACE2, "particle_trace2" },
	{ PARTICLE_SMOKE, "particle_smoke" },
	{ PARTICLE_EXPLOSION1, "explosion_o" },
	{ PARTICLE_EXPLOSION2, "explosion_big" },
	{ PARTICLE_EXPLOSION_G, "explosion_g" },
	{ PARTICLE_EXPLOSION_E, "explosion_e" },
	{ PARTICLE_EXPLOSION_S, "explosion_s" },
	{ PARTICLE_EXPLOSION_P, "explosion_plazma" },
	{ PARTICLE_BIGBLAST, "bigblast" },
	{ PARTICLE_SMALLBLAST, "smallblast" },
	{ PARTICLE_GAUSS1, "particle_gauss1" },
	{ PARTICLE_GAUSS2, "particle_gauss2" },
	{ PARTICLE_GAUSS_HIT, "particle_gausshit" },
	{ PARTICLE_GREEN, "particle_green" },
	{ PARTICLE_YELLOW, "particle_yellow" },
	{ PARTICLE_CATTRACK, "cat_track" },
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
		vec2d dir = particle.GetRotationSpeed() > 0 ? vec2d(particle.GetRotationSpeed() * particle.GetTime()) : particle.GetDirection();
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
		float size = particle.GetSizeOverride();
		if( size < 0 )
			dc.DrawSprite(texId, frame, color, pos.x, pos.y, dir);
		else
			dc.DrawSprite(texId, frame, color, pos.x, pos.y, size, size, dir);
	}
}
