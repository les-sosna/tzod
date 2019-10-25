#pragma once
#include "MovingObject.h"

enum DecalType
{
	PARTICLE_FIRE1,
	PARTICLE_FIRE2,
	PARTICLE_FIRE3,
	PARTICLE_FIRE4,
	PARTICLE_FIRESPARK,
	PARTICLE_TYPE1,
	PARTICLE_TYPE2,
	PARTICLE_TYPE3,
	PARTICLE_TRACE1,
	PARTICLE_TRACE2,
	PARTICLE_SMOKE,
	PARTICLE_EXPLOSION1,
	PARTICLE_EXPLOSION2,
	PARTICLE_EXPLOSION_G,
	PARTICLE_EXPLOSION_E,
	PARTICLE_EXPLOSION_S,
	PARTICLE_EXPLOSION_P,
	PARTICLE_BIGBLAST,
	PARTICLE_SMALLBLAST,
	PARTICLE_GAUSS1,
	PARTICLE_GAUSS2,
	PARTICLE_GAUSS_HIT,
	PARTICLE_GREEN,
	PARTICLE_YELLOW,
	PARTICLE_CATTRACK,
};

#define GC_FLAG_DECAL_FADE            (GC_FLAG_MO_ << 0)
#define GC_FLAG_DECAL_                (GC_FLAG_MO_ << 1)

class GC_Decal : public GC_MovingObject
{
	DECLARE_SELF_REGISTRATION(GC_Decal);
public:
	GC_Decal(vec2d pos, DecalType ptype, float lifeTime, float age = 0);
	GC_Decal(FromFile);

	DecalType GetDecalType() const { return _dtype; }
	bool GetFade() const { return CheckFlags(GC_FLAG_DECAL_FADE); }
	float GetTimeCreated() const { return _timeCreated; }
	float GetLifeTime() const { return _timeLife; }
	float GetRotationSpeed() const { return _rotationSpeed; }
	float GetSizeOverride() const { return _sizeOverride; }

	void SetFade(bool fade) { SetFlags(GC_FLAG_DECAL_FADE, fade); }
	void SetAutoRotate(float speed) { _rotationSpeed = speed; }
	void SetSizeOverride(float size) { _sizeOverride = size; }

	// GC_Object
	void Init(World &world) override;
	void Resume(World &world) override;
	void Serialize(World &world, SaveFile &f) override;

private:
	DecalType _dtype = PARTICLE_TYPE1;
	float _sizeOverride = -1;
	float _timeCreated;
	float _timeLife;
	float _rotationSpeed;
};

///////////////////////////////////////////////////////////////////////////////
// Workaround to assign different renderers

#define DECLARE_DECAL(clsname)                                          \
    class clsname : public GC_Decal                                     \
    {                                                                   \
        DECLARE_SELF_REGISTRATION(clsname);                             \
    public:                                                             \
        using GC_Decal::GC_Decal;                                       \
    };

DECLARE_DECAL(GC_DecalExplosion);
DECLARE_DECAL(GC_DecalGauss);
